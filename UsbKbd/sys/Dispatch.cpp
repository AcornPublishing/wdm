//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	UsbKbd example
/////////////////////////////////////////////////////////////////////////////
//	dispatch.cpp:		I/O IRP handlers
/////////////////////////////////////////////////////////////////////////////
//	UsbKbdCreate			Handle Create/Open file IRP
//	UsbKbdClose			Handle Close file IRPs
//	UsbKbdRead			Handle Read IRPs
//	UsbKbdWrite			Handle Write IRPs
//	UsbKbdDeviceControl	Handle DeviceIoControl IRPs
//	UsbKbdSystemControl	Handle WMI IRPs
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "UsbKbd.h"
#include "Ioctl.h"

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdCreate:
//
//	Description:
//		Handle IRP_MJ_CREATE requests
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			IrpStack->Parameters.Create.xxx has create parameters
//			IrpStack->FileObject->FileName has file name of device
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdCreate(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx = (PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DebugPrint( "Create File is %T", &(IrpStack->FileObject->FileName));

	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);

	InterlockedIncrement(&dx->OpenHandleCount);

	// Display USB host controller driver info
	USBD_VERSION_INFORMATION vi;
	USBD_GetUSBDIVersion(&vi);
	DebugPrint("USBDI version %x Supported version %x", vi.USBDI_Version, vi.Supported_USB_Version);

	// Reset device and select HID keyboard configuration
	UsbResetDevice(dx);
	UsbSelectConfiguration(dx);

	// DebugPrint USB info
	UsbGetUsbInfo(dx);

	// DebugPrint string lang ids
	ULONG StringsLen = 50;
	PVOID pvstrings;
	NTSTATUS status = UsbGetSpecifiedDescriptor( dx, pvstrings, USB_STRING_DESCRIPTOR_TYPE, StringsLen);
	if( NT_SUCCESS(status))
	if( StringsLen>0)
	{
		PUSB_STRING_DESCRIPTOR strings = (PUSB_STRING_DESCRIPTOR)pvstrings;
		ULONG LangIds = (strings->bLength - FIELD_OFFSET( USB_STRING_DESCRIPTOR, bString[0]))>>1;
		DebugPrint("StringsLen %d LangIds %d",StringsLen,LangIds);
		if( LangIds>0)
			for( ULONG LangIdNo=0; LangIdNo<LangIds; LangIdNo++)
				DebugPrint("LangId[%d]=%4x",LangIdNo,strings->bString[LangIdNo]);
	}
	else
		DebugPrintMsg("No string descriptors");
	FreeIfAllocated(pvstrings);

	// Out of interest, get the idle rate
	UsbGetIdleRate(dx);

	// Complete successfully
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdClose:
//
//	Description:
//		Handle IRP_MJ_CLOSE requests
//		Allow closes to complete if device not started
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdClose(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx = (PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("Close");

	InterlockedDecrement(&dx->OpenHandleCount);

	UsbDeselectConfiguration(dx);

	// Complete successfully
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}
 
/////////////////////////////////////////////////////////////////////////////
//	UsbKbdRead:
//
//	Description:
//		Handle IRP_MJ_READ requests
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			IrpStack->Parameters.Read.xxx has read parameters
//			User buffer at:	AssociatedIrp.SystemBuffer	(buffered I/O)
//							MdlAddress					(direct I/O)
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdRead(	IN PDEVICE_OBJECT fdo,
						IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx = (PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG BytesTxd = 0;

	// Get call parameters
	LONGLONG FilePointer = IrpStack->Parameters.Read.ByteOffset.QuadPart;
	ULONG ReadLen = IrpStack->Parameters.Read.Length;
	DebugPrint("Read %d bytes from file pointer %d",(int)ReadLen,(int)FilePointer);

	// Check file pointer
	if( FilePointer<0)
		status = STATUS_INVALID_PARAMETER;
	else
	{
		status = UsbDoInterruptTransfer( dx, Irp->AssociatedIrp.SystemBuffer, ReadLen);
		BytesTxd = ReadLen;
	}
	DebugPrint("Read: %x %d bytes returned",status,(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdWrite:
//
//	Description:
//		Handle IRP_MJ_WRITE requests
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			IrpStack->Parameters.Write.xxx has write parameters
//			User buffer at:	AssociatedIrp.SystemBuffer	(buffered I/O)
//							MdlAddress					(direct I/O)
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdWrite(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx = (PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG BytesTxd = 0;

	// Get call parameters
	LONGLONG FilePointer = IrpStack->Parameters.Write.ByteOffset.QuadPart;
	ULONG WriteLen = IrpStack->Parameters.Write.Length;
	DebugPrint("Write %d bytes from file pointer %d",(int)WriteLen,(int)FilePointer);

	if( FilePointer<0 || WriteLen<1)
		status = STATUS_INVALID_PARAMETER;
	else
	{
		// Only ever write one byte
		BytesTxd = 1;
		PUCHAR pData = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
		UsbSendOutputReport( dx, *pData);
	}

	DebugPrint("Write: %d bytes written",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdDeviceControl:
//
//	Description:
//		Handle IRP_MJ_DEVICE_CONTROL requests
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			Buffered:	AssociatedIrp.SystemBuffer (and IrpStack->Parameters.DeviceIoControl.Type3InputBuffer)
//			Direct:		MdlAddress
//
//			IrpStack->Parameters.DeviceIoControl.InputBufferLength
//			IrpStack->Parameters.DeviceIoControl.OutputBufferLength 
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdDeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx = (PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG BytesTxd = 0;

	ULONG ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	DebugPrint("DeviceIoControl: Control code %x InputLength %d OutputLength %d",
				ControlCode, InputLength, OutputLength);

	switch( ControlCode)
	{
	///////	Set read timeout
	case IOCTL_USBKBD_SET_READ_TIMEOUT:
		if( InputLength!=4)
			status = STATUS_INVALID_PARAMETER;
		else
		{
			dx->UsbTimeout = *((PULONG)Irp->AssociatedIrp.SystemBuffer);
			DebugPrint( "USB timeout set to %d",dx->UsbTimeout);
		}
		break;

	///////	Get device descriptor
	case IOCTL_USBKBD_GET_DEVICE_DESCRIPTOR:
	{
		PUSB_DEVICE_DESCRIPTOR deviceDescriptor = NULL;
		ULONG size;
		NTSTATUS status = UsbGetDeviceDescriptor( dx, deviceDescriptor, size);
		if( NT_SUCCESS(status))
		{
			BytesTxd = size;
			if( BytesTxd>OutputLength)
				BytesTxd = OutputLength;
			RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,deviceDescriptor,BytesTxd);
		}
		FreeIfAllocated(deviceDescriptor);
		break;
	}

	///////	Get configuration and other descriptors
	case IOCTL_USBKBD_GET_CONFIGURATION_DESCRIPTORS:
	{
		PUSB_CONFIGURATION_DESCRIPTOR Descriptors = NULL;
		ULONG size;
		NTSTATUS status = UsbGetConfigurationDescriptors( dx, Descriptors, 0, size);
		if( NT_SUCCESS(status))
		{
			BytesTxd = size;
			if( BytesTxd>OutputLength)
				BytesTxd = OutputLength;
			RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,Descriptors,BytesTxd);
		}
		FreeIfAllocated(Descriptors);
		break;
	}

	///////	Get HID Report descriptor
	case IOCTL_USBKBD_GET_SPECIFIED_DESCRIPTOR:
	{
		if( InputLength!=8)
			status = STATUS_INVALID_PARAMETER;
		else
		{
			PULONG pData = (PULONG)Irp->AssociatedIrp.SystemBuffer;
			UCHAR type = (UCHAR)*pData++;
			ULONG size = *pData;
			PVOID Descriptor = NULL;
			NTSTATUS status = UsbGetSpecifiedDescriptor( dx, Descriptor, type, size);
			if( NT_SUCCESS(status))
			{
				BytesTxd = size;
				if( BytesTxd>OutputLength)
					BytesTxd = OutputLength;
				RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,Descriptor,BytesTxd);
			}
			FreeIfAllocated(Descriptor);
		}
		break;
	}

	///////	Get Statuses
	case IOCTL_USBKBD_GET_STATUSES:
	{
		UCHAR Statuses[6];
		ULONG size = sizeof(Statuses);
		NTSTATUS status = UsbGetStatuses( dx, Statuses, size);
		if( NT_SUCCESS(status))
		{
			BytesTxd = size;
			if( BytesTxd>OutputLength)
				BytesTxd = OutputLength;
			RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,Statuses,BytesTxd);
		}
		break;
	}

	///////	Get Frame info
	case IOCTL_USBKBD_GET_FRAME_INFO:
	{
		ULONG Info[3];
		NTSTATUS status = UsbGetFrameInfo( dx, Info[0], Info[1], Info[2]);
		if( NT_SUCCESS(status))
		{
			BytesTxd = sizeof(Info);
			if( BytesTxd>OutputLength)
				BytesTxd = OutputLength;
			RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,Info,BytesTxd);
		}
		break;
	}

	///////	Invalid request
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
	}

	DebugPrint("DeviceIoControl: %d bytes written",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdSystemControl:
//
//	Description:
//		Handle IRP_MJ_SYSTEM_CONTROL requests
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			Various minor parameters
//			IrpStack->Parameters.WMI.xxx has WMI parameters
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdSystemControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx = (PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	DebugPrintMsg("SystemControl");

	// Just pass to lower driver
	IoSkipCurrentIrpStackLocation(Irp);
	NTSTATUS status = IoCallDriver( dx->NextStackDevice, Irp);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdCleanup:
//
//	Description:
//		Handle IRP_MJ_CLEANUP requests
//		Cancel queued IRPs which match given FileObject
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			IrpStack->FileObject has handle to file
//
//	Return Value:
//		This function returns STATUS_XXX

//	Not needed for UsbKbd

/////////////////////////////////////////////////////////////////////////////
//	CompleteIrp:	Sets IoStatus and completes the IRP

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
