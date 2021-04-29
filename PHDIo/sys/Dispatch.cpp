//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	PHDIo example
/////////////////////////////////////////////////////////////////////////////
//	dispatch.cpp:		I/O IRP handlers
/////////////////////////////////////////////////////////////////////////////
//	PHDIoCreate			Handle Create/Open file IRP
//*	ClaimResources		Ask kernel to give is the resources we want
//*	UnclaimResources	Release our resources back to the system
//*	TranslateAndMapResources	Translate and map our resources
//*	FreeResources		Unmap memory and disconnect from our interrupt
//*	GetResourcesFromFilename	Extract required resources from filename string
//*	usStrCmpN			Compare section of Unicode string with a wide string
//*	usGetHex			Get hex value from section of Unicode string
//*	usGetDec			Get decimal value from section of Unicode string
//	PHDIoClose			Handle Close file IRPs
//	PHDIoRead			Handle Read IRPs
//	PHDIoWrite			Handle Write IRPs
//	PHDIoDeviceControl	Handle DeviceIoControl IRPs
//	PHDIoSystemControl	Handle WMI IRPs
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	13-Jan-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "PHDIo.h"
#include "Ioctl.h"

/////////////////////////////////////////////////////////////////////////////

NTSTATUS GetResourcesFromFilename( IN PUNICODE_STRING usfilename, IN PPHDIO_DEVICE_EXTENSION dx);
bool usStrCmpN( PUNICODE_STRING us, int& pos, int maxchars, PWSTR cmp, int cmpchars);
bool usGetHex( PUNICODE_STRING us, int& pos, int maxchars, ULONG& value);
bool usGetDec( PUNICODE_STRING us, int& pos, int maxchars, ULONG& value);

NTSTATUS ClaimResources( IN PDEVICE_OBJECT phddo);
NTSTATUS TranslateAndMapResources( IN PDEVICE_OBJECT phddo);
void UnclaimResources( IN PDEVICE_OBJECT phddo);


NTSTATUS PowerUpDevice( IN PDEVICE_OBJECT phddo);

/////////////////////////////////////////////////////////////////////////////
//	PHDIoCreate:
//
//	Description:
//		Handle IRP_MJ_CREATE requests
//
//	Arguments:
//		Pointer to our PHD DO
//		Pointer to the IRP
//			IrpStack->Parameters.Create.xxx has create parameters
//			IrpStack->FileObject->FileName has file name of device
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS PHDIoCreate(	IN PDEVICE_OBJECT phddo,
						IN PIRP Irp)
{
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DebugPrint( "Create File is %T", &(IrpStack->FileObject->FileName));

	dx->GotPortOrMemory = false;
	dx->GotInterrupt = false;
	dx->PortNeedsMapping = false;
	dx->ConnectedToInterrupt = false;
	dx->ResourceOverride = FALSE;

	// Get resources from filename string
	PUNICODE_STRING usfilename = &(IrpStack->FileObject->FileName);
	NTSTATUS status = GetResourcesFromFilename(usfilename,dx);
	if( !NT_SUCCESS(status)) goto fail;

	// We must have IO port resource
	if( !dx->GotPortOrMemory)
	{
		DebugPrintMsg("No IO Port resource in filename");
		status = STATUS_INVALID_PARAMETER;
		goto fail;
	}

	// Claim resources
	status = ClaimResources(phddo);
	if( !NT_SUCCESS(status))
	{
		DebugPrintMsg("Could not ClaimResources");
		goto fail;
	}

	// Translate and map resources
	status = TranslateAndMapResources(phddo);
	if( !NT_SUCCESS(status))
	{
		UnclaimResources(phddo);
		goto fail;
	}

	// Complete
	return CompleteIrp(Irp,status);

	// On error, make sure everything's off
fail:
	dx->GotPortOrMemory = false;
	dx->GotInterrupt = false;
	dx->PortNeedsMapping = false;
	dx->ConnectedToInterrupt = false;
	return CompleteIrp(Irp,status);
}

/////////////////////////////////////////////////////////////////////////////
//	ClaimResources:	Ask kernel to give is the resources we want

NTSTATUS ClaimResources( IN PDEVICE_OBJECT phddo)
{
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;

	// Get resource count: either 1 (IOport) or 2 (IOport&IRQ)
	ULONG PartialResourceCount = 1;
	if( dx->GotInterrupt) PartialResourceCount++;

	// Get size of required CM_RESOURCE_LIST
	ULONG ListSize = FIELD_OFFSET( CM_RESOURCE_LIST, List[0]);

	ListSize += sizeof( CM_FULL_RESOURCE_DESCRIPTOR) +
				((PartialResourceCount-1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));

	// Allocate CM_RESOURCE_LIST
	PCM_RESOURCE_LIST ResourceList = (PCM_RESOURCE_LIST)ExAllocatePool( PagedPool, ListSize);
	if( ResourceList==NULL)
	{
		DebugPrintMsg("Cannot allocate memory for ResourceList");
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory( ResourceList, ListSize);

	// Only one Full Resource Descriptor needed, for ISA
	ResourceList->Count = 1;

	// Initialise Full Resource Descriptor
	PCM_FULL_RESOURCE_DESCRIPTOR FullRD = &ResourceList->List[0];
	FullRD->InterfaceType = Isa;
	FullRD->BusNumber = 0;

	FullRD->PartialResourceList.Count = PartialResourceCount;

	// Initialise Partial Resource Descriptor for IO port
	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = &FullRD->PartialResourceList.PartialDescriptors[0];
	resource->Type = CmResourceTypePort;
	resource->ShareDisposition = CmResourceShareDriverExclusive;
	resource->Flags = CM_RESOURCE_PORT_IO;
	resource->u.Port.Start = dx->PortStartAddress;
	resource->u.Port.Length = dx->PortLength;

	// Initialise Partial Resource Descriptor for Interrupt
	if( dx->GotInterrupt)
	{
		resource++;
		resource->Type = CmResourceTypeInterrupt;
		resource->ShareDisposition = CmResourceShareDriverExclusive;
		if( dx->Mode==Latched)
			resource->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
		else
			resource->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
		resource->u.Interrupt.Level = dx->Irql;
		resource->u.Interrupt.Vector = dx->Irql;
		resource->u.Interrupt.Affinity = 1;
	}

	// Ask for resources for the driver
	DebugPrint("Allocating %d resources",PartialResourceCount);
	DebugPrint("phddo->DriverObject %x",phddo->DriverObject);
	if( dx->ResourceOverride) DebugPrintMsg("Resource override conflict");
	BOOLEAN ConflictDetected;
	NTSTATUS status = IoReportResourceUsage( NULL,
						phddo->DriverObject, ResourceList, ListSize,	// Driver resources
						NULL, NULL, 0,	// Device resources
						dx->ResourceOverride, &ConflictDetected);
	// Cope (or override) if resource conflict found
	if( ConflictDetected)
	{
		DebugPrintMsg("ConflictDetected");
		// NT4 returns STATUS_SUCCESS;  W2000 returns !NT_SUCCESS
		if( dx->ResourceOverride)
		{
			DebugPrintMsg("Conflict detected and overridden");
			status = STATUS_SUCCESS;
		}
	}
	// Free allocated memory
	ExFreePool(ResourceList);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UnclaimResources:	Release our resources back to the system

void UnclaimResources( IN PDEVICE_OBJECT phddo)
{
	DebugPrintMsg("Freeing all allocated resources");
	// Release all driver's resources by declaring we have none.
	CM_RESOURCE_LIST ResourceList;
	ResourceList.Count = 0;
	BOOLEAN ConflictDetected;
	IoReportResourceUsage( NULL,
					phddo->DriverObject, &ResourceList, sizeof(ResourceList),	// Driver resources
					NULL, NULL, 0,	// Device resources
					FALSE, &ConflictDetected);
	// ignore return result
}

/////////////////////////////////////////////////////////////////////////////
//	TranslateAndMapResources:	Translate and map our resources

NTSTATUS TranslateAndMapResources( IN PDEVICE_OBJECT phddo)
{
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;

	// Translate IO port values
	ULONG AddressSpace = 1;	// IO space
	if( !HalTranslateBusAddress( Isa, 0, dx->PortStartAddress,
					&AddressSpace, &dx->PortStartAddress))
	{
		DebugPrint( "Create file: could not translate IO %x", dx->PortStartAddress.LowPart);
		return STATUS_INVALID_PARAMETER;
	}
	DebugPrint( "IO trans %x,%d", dx->PortStartAddress.LowPart, dx->PortLength);
	dx->PortNeedsMapping = (AddressSpace==0);
	dx->PortInIOSpace = (AddressSpace==1);

	// Translate IRQ values
	if( dx->GotInterrupt)
	{
		ULONG irq = dx->Irql;
		dx->Vector = HalGetInterruptVector( Isa, 0, irq, irq, &dx->Irql, &dx->Affinity);
		if( dx->Vector==NULL)
		{
			DebugPrint( "Create filename: Could not get interrupt vector for IRQ %d", irq);
			return STATUS_INVALID_PARAMETER;
		}
		DebugPrint("Interrupt vector %x IRQL %d Affinity %d Mode %d",
						dx->Vector, dx->Irql, dx->Affinity, dx->Mode);
	}

	// Map memory
	if( dx->PortNeedsMapping)
	{
		dx->PortBase = (PUCHAR)MmMapIoSpace( dx->PortStartAddress, dx->PortLength, MmNonCached);
		if( dx->PortBase==NULL)
		{
			DebugPrintMsg( "Cannot map IO port");
			return STATUS_NO_MEMORY;
		}
	}
	else
		dx->PortBase = (PUCHAR)dx->PortStartAddress.LowPart;

	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//	FreeResources:	Unmap memory and disconnect from our interrupt

void FreeResources( IN PDEVICE_OBJECT phddo)
{
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;

	// Unmap memory
	if (dx->PortNeedsMapping)
	{
		DebugPrintMsg( "Unmapping memory");
		MmUnmapIoSpace( (PVOID)dx->PortBase, dx->PortLength);
		dx->PortNeedsMapping = false;
	}

	// Disconnect from interrupt
	if( dx->ConnectedToInterrupt)
	{
		DebugPrintMsg( "Disconnecting from interrupt");
		IoDisconnectInterrupt( dx->InterruptObject);
		dx->ConnectedToInterrupt = false;
	}
}

/////////////////////////////////////////////////////////////////////////////
//	GetResourcesFromFilename:	Extract required resources from filename string
//								All letters in filename must be lower case

NTSTATUS GetResourcesFromFilename( IN PUNICODE_STRING usfilename, IN PPHDIO_DEVICE_EXTENSION dx)
{
	NTSTATUS status = STATUS_SUCCESS;
	if( usfilename->Length==0)
		return STATUS_INVALID_PARAMETER;

	int chars = usfilename->Length>>1;

	// Check \isa given at start
	int uspos = 0;
	if( !usStrCmpN( usfilename, uspos, chars, L"\\isa", 4))
	{
		DebugPrintMsg( "Create filename does not start with '\\isa'");
		return STATUS_INVALID_PARAMETER;
	}

	/////////////////////////////////////////////////////////////////////////
	// Search for I/O space and IRQ resource strings

	while(uspos!=chars)
	{
		/////////////////////////////////////////////////////////////////////
		// Get "\io<base>,<length>" in hex
		if( usStrCmpN( usfilename, uspos, chars, L"\\io", 3))
		{
			// Get <base>
			ULONG io;
			if( !usGetHex( usfilename, uspos, chars, io))
			{
				DebugPrintMsg( "Create filename '\\io' not followed by base in hex");
				return STATUS_INVALID_PARAMETER;
			}
			// Get comma
			if( !usStrCmpN( usfilename, uspos, chars, L",", 1))
			{
				DebugPrintMsg( "Create filename '\\io<base>' not followed by comma");
				return STATUS_INVALID_PARAMETER;
			}
			// Get <length>
			ULONG iolen;
			if( !usGetHex( usfilename, uspos, chars, iolen))
			{
				DebugPrintMsg( "Create filename '\\io<base>,' not followed by length in hex");
				return STATUS_INVALID_PARAMETER;
			}

			// Save raw values
			DebugPrint( "IO raw   %x,%d", io, iolen);
			dx->PortStartAddress.QuadPart = io;
			dx->PortLength = iolen;
			dx->GotPortOrMemory = true;
		}
		/////////////////////////////////////////////////////////////////////
		// Get "\irq<number>" in decimal
		else if( usStrCmpN( usfilename, uspos, chars, L"\\irq", 4))
		{
			// Get <number>
			ULONG irq;
			if( !usGetDec( usfilename, uspos, chars, irq) || irq>15)
			{
				DebugPrintMsg( "Create filename '\\irq' not followed by IRQ number in decimal");
				return STATUS_INVALID_PARAMETER;
			}
			DebugPrint( "IRQ %d", irq);
			dx->Irql = (KIRQL)irq;
			dx->Mode = Latched;
			dx->GotInterrupt = true;
		}
		else if( usStrCmpN( usfilename, uspos, chars, L"\\override", 9))
		{
			DebugPrintMsg("ResourceOverride");
			dx->ResourceOverride = TRUE;
		}
		else
		{
			DebugPrint( "Create filename:  Unrecognised resource %*S",
				chars-uspos, &usfilename->Buffer[uspos]);
			return STATUS_INVALID_PARAMETER;
		}
	}
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	usStrCmpN:	Compare section of Unicode string with a wide string

bool usStrCmpN( PUNICODE_STRING us, int& pos, int maxchars, PWSTR cmp, int cmpchars)
{
	if( pos+cmpchars>maxchars) return false;
	for( int cmpno=0; cmpno<cmpchars; cmpno++)
		if( us->Buffer[pos+cmpno] != cmp[cmpno])
			return false;
	pos += cmpchars;
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	usGetHex:	Get hex value from section of Unicode string

bool usGetHex( PUNICODE_STRING us, int& pos, int maxchars, ULONG& value)
{
	if( pos==maxchars) return false;
	value = 0;
	while( pos<maxchars)
	{
		wchar_t ch = us->Buffer[pos];
		ULONG ThisDigit = 0;
		if( ch>=L'0' && ch<=L'9')
			ThisDigit = ch-L'0';
		else if( ch>=L'a' && ch<=L'f')
			ThisDigit = ch-L'a'+10;
		else
			break;
		value = (value<<4)+ThisDigit;
		pos++;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	usGetDec:	Get decimal value from section of Unicode string

bool usGetDec( PUNICODE_STRING us, int& pos, int maxchars, ULONG& value)
{
	if( pos==maxchars) return false;
	value = 0;
	while( pos<maxchars)
	{
		wchar_t ch = us->Buffer[pos];
		ULONG ThisDigit = 0;
		if( ch>=L'0' && ch<=L'9')
			ThisDigit = ch-L'0';
		else
			break;
		value = (value*10)+ThisDigit;
		pos++;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	PHDIoClose:
//
//	Description:
//		Handle IRP_MJ_CLOSE requests
//		Allow closes to complete if device not started
//
//	Arguments:
//		Pointer to our PHD DO
//		Pointer to the IRP
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS PHDIoClose(IN PDEVICE_OBJECT phddo,
					IN PIRP Irp)
{
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;
	DebugPrintMsg("Close");

	FreeResources(phddo);

	UnclaimResources(phddo);

	// Complete successfully
	return CompleteIrp(Irp,STATUS_SUCCESS);
}
 
/////////////////////////////////////////////////////////////////////////////
//	PHDIoRead:
//
//	Description:
//		Handle IRP_MJ_READ requests
//
//	Arguments:
//		Pointer to our PHD DO
//		Pointer to the IRP
//			IrpStack->Parameters.Read.xxx has read parameters
//			User buffer at:	AssociatedIrp.SystemBuffer	(buffered I/O)
//							MdlAddress					(direct I/O)
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS PHDIoRead(	IN PDEVICE_OBJECT phddo,
					IN PIRP Irp)
{
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;
	if( !dx->GotPortOrMemory)
		return CompleteIrp( Irp, STATUS_INSUFFICIENT_RESOURCES);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG BytesTxd = 0;

	// Get call parameters
	LONGLONG FilePointer = IrpStack->Parameters.Read.ByteOffset.QuadPart;
	ULONG ReadLen = IrpStack->Parameters.Read.Length;
	DebugPrint("Read %d bytes from file pointer %d",(int)ReadLen,(int)FilePointer);

	// Check file pointer
	NTSTATUS status;
	if( FilePointer!=0)
		status = STATUS_INVALID_PARAMETER;
	else
	{
		IoMarkIrpPending(Irp);
		IoStartPacket( phddo, Irp, 0, PHDIoCancelIrp);
		return STATUS_PENDING;
	}
	DebugPrint("Read: %d bytes returned",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	PHDIoWrite:
//
//	Description:
//		Handle IRP_MJ_WRITE requests
//
//	Arguments:
//		Pointer to our PHD DO
//		Pointer to the IRP
//			IrpStack->Parameters.Write.xxx has write parameters
//			User buffer at:	AssociatedIrp.SystemBuffer	(buffered I/O)
//							MdlAddress					(direct I/O)
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS PHDIoWrite(	IN PDEVICE_OBJECT phddo,
						IN PIRP Irp)
{
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;
	if( !dx->GotPortOrMemory)
		return CompleteIrp( Irp, STATUS_INSUFFICIENT_RESOURCES);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG BytesTxd = 0;

	// Get call parameters
	LONGLONG FilePointer = IrpStack->Parameters.Write.ByteOffset.QuadPart;
	ULONG WriteLen = IrpStack->Parameters.Write.Length;
	DebugPrint("Write %d bytes from file pointer %d",(int)WriteLen,(int)FilePointer);

	NTSTATUS status;
	if( FilePointer!=0)
		status = STATUS_INVALID_PARAMETER;
	else
	{
		IoMarkIrpPending(Irp);
		IoStartPacket( phddo, Irp, 0, PHDIoCancelIrp);
		return STATUS_PENDING;
	}

	DebugPrint("Write: %d bytes written",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	PHDIoDeviceControl:
//
//	Description:
//		Handle IRP_MJ_DEVICE_CONTROL requests
//
//	Arguments:
//		Pointer to our PHD DO
//		Pointer to the IRP
//			Buffered:	AssociatedIrp.SystemBuffer (and IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)
//			Direct:		MdlAddress
//
//			IrpStack->Parameters.DeviceIoControl.InputBufferLength
//			IrpStack->Parameters.DeviceIoControl.OutputBufferLength 
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS PHDIoDeviceControl(	IN PDEVICE_OBJECT phddo,
								IN PIRP Irp)
{
	PPHDIO_DEVICE_EXTENSION dx = (PPHDIO_DEVICE_EXTENSION)phddo->DeviceExtension;
	if( !dx->GotPortOrMemory)
		return CompleteIrp( Irp, STATUS_INSUFFICIENT_RESOURCES);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

	ULONG ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	DebugPrint("DeviceIoControl: Control code %x InputLength %d OutputLength %d",
				ControlCode, InputLength, OutputLength);

	NTSTATUS status;
	switch( ControlCode)
	{
	///////	Pass to StartIo
	case IOCTL_PHDIO_RUN_CMDS:
	case IOCTL_PHDIO_CMDS_FOR_READ:
	case IOCTL_PHDIO_CMDS_FOR_READ_START:
	case IOCTL_PHDIO_CMDS_FOR_WRITE:
	case IOCTL_PHDIO_GET_RW_RESULTS:
//		DebugPrintMsg("DeviceIoControl: StartIo called");
		IoMarkIrpPending(Irp);
		IoStartPacket( phddo, Irp, 0, PHDIoCancelIrp);
		return STATUS_PENDING;
	///////	Invalid request
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
	}

	DebugPrintMsg("DeviceIoControl: invalid request code");

	// Complete IRP
	CompleteIrp(Irp,status);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	CompleteIrp:	Sets IoStatus and completes the IRP

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info/*=0*/, IN CCHAR PriorityBoost/*=IO_NO_INCREMENT*/)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp,PriorityBoost);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
