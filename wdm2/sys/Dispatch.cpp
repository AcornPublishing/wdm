//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm2 example
/////////////////////////////////////////////////////////////////////////////
//	dispatch.cpp:		I/O IRP handlers
/////////////////////////////////////////////////////////////////////////////
//	Wdm2Create			Handle Create/Open file IRP
//	Wdm2Close			Handle Close file IRPs
//	Wdm2Read			Handle Read IRPs
//	Wdm2Write			Handle Write IRPs
//	Wdm2DeviceControl	Handle DeviceIoControl IRPs
//	Wdm2SystemControl	Handle WMI IRPs
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "wdm2.h"
#include "Ioctl.h"

/////////////////////////////////////////////////////////////////////////////

NTSTATUS PowerUpDevice( IN PDEVICE_OBJECT fdo);

/////////////////////////////////////////////////////////////////////////////
//	Buffer and BufferSize and guarding spin lock globals (in unpaged memory)

KSPIN_LOCK BufferLock;
PUCHAR	Buffer = NULL;
ULONG	BufferSize = 0;

/////////////////////////////////////////////////////////////////////////////
//	Wdm2Create:
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

NTSTATUS Wdm2Create(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DebugPrint( "Create File is %T", &(IrpStack->FileObject->FileName));

	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);

	InterlockedIncrement(&dx->OpenHandleCount);

	// Complete successfully
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}

/////////////////////////////////////////////////////////////////////////////
//	Wdm2Close:
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

NTSTATUS Wdm2Close(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("Close");

	InterlockedDecrement(&dx->OpenHandleCount);

	// Complete successfully
	return CompleteIrp(Irp,STATUS_SUCCESS,0);
}
 
/////////////////////////////////////////////////////////////////////////////
//	Wdm2Read:
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

NTSTATUS Wdm2Read(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	NTSTATUS status = PowerUpDevice(fdo);
	if( !NT_SUCCESS(status))
		return CompleteIrp(Irp, status, 0);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	LONG BytesTxd = 0;

	// Get call parameters
	LONGLONG FilePointer = IrpStack->Parameters.Read.ByteOffset.QuadPart;
	ULONG ReadLen = IrpStack->Parameters.Read.Length;
	DebugPrint("Read %d bytes from file pointer %d",(int)ReadLen,(int)FilePointer);

	// Get access to the shared buffer
	KIRQL irql;
	KeAcquireSpinLock(&BufferLock,&irql);

	// Check file pointer
	if( FilePointer<0)
		status = STATUS_INVALID_PARAMETER;
	if( FilePointer>=(LONGLONG)BufferSize)
		status = STATUS_END_OF_FILE;

	if( status==STATUS_SUCCESS)
	{
		// Get transfer count
		if( ((ULONG)FilePointer)+ReadLen>BufferSize)
		{
			BytesTxd = BufferSize - (ULONG)FilePointer;
			if( BytesTxd<0) BytesTxd = 0;
		}
		else
			BytesTxd = ReadLen;

		// Read from shared buffer
		if( BytesTxd>0 && Buffer!=NULL)
			RtlCopyMemory( Irp->AssociatedIrp.SystemBuffer, Buffer+FilePointer, BytesTxd);
	}

	// Release shared buffer
	KeReleaseSpinLock(&BufferLock,irql);

	DebugPrint("Read: %d bytes returned",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	Wdm2Write:
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

NTSTATUS Wdm2Write(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	NTSTATUS status = PowerUpDevice(fdo);
	if( !NT_SUCCESS(status))
		return CompleteIrp(Irp, status, 0);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	LONG BytesTxd = 0;

	// Get call parameters
	LONGLONG FilePointer = IrpStack->Parameters.Write.ByteOffset.QuadPart;
	ULONG WriteLen = IrpStack->Parameters.Write.Length;
	DebugPrint("Write %d bytes from file pointer %d",(int)WriteLen,(int)FilePointer);

	if( FilePointer<0)
		status = STATUS_INVALID_PARAMETER;
	else
	{
		// Get access to the shared buffer
		KIRQL irql;
		KeAcquireSpinLock(&BufferLock,&irql);

		BytesTxd = WriteLen;

		// (Re)allocate buffer if necessary
		if( ((ULONG)FilePointer)+WriteLen>BufferSize)
		{
			ULONG NewBufferSize = ((ULONG)FilePointer)+WriteLen;
			PVOID NewBuffer = ExAllocatePool(NonPagedPool,NewBufferSize);
			if( NewBuffer==NULL)
			{
				BytesTxd = BufferSize - (ULONG)FilePointer;
				if( BytesTxd<0) BytesTxd = 0;
			}
			else
			{
				RtlZeroMemory(NewBuffer,NewBufferSize);
				if( Buffer!=NULL)
				{
					RtlCopyMemory(NewBuffer,Buffer,BufferSize);
					ExFreePool(Buffer);
				}
				Buffer = (PUCHAR)NewBuffer;
				BufferSize = NewBufferSize;
			}
		}

		// Write to shared memory
		if( BytesTxd>0 && Buffer!=NULL)
			RtlCopyMemory( Buffer+FilePointer, Irp->AssociatedIrp.SystemBuffer, BytesTxd);

		// Release shared buffer
		KeReleaseSpinLock(&BufferLock,irql);
	}

	DebugPrint("Write: %d bytes written",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	Wdm2DeviceControl:
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

NTSTATUS Wdm2DeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	NTSTATUS status = PowerUpDevice(fdo);
	if( !NT_SUCCESS(status))
		return CompleteIrp(Irp, status, 0);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG BytesTxd = 0;

	ULONG ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	DebugPrint("DeviceIoControl: Control code %x InputLength %d OutputLength %d",
				ControlCode, InputLength, OutputLength);

	// Get access to the shared buffer
	KIRQL irql;
	KeAcquireSpinLock(&BufferLock,&irql);
	switch( ControlCode)
	{
	///////	Zero Buffer
	case IOCTL_WDM1_ZERO_BUFFER:
		// Zero the buffer
		if( Buffer!=NULL && BufferSize>0)
			RtlZeroMemory(Buffer,BufferSize);
		break;

	///////	Remove Buffer
	case IOCTL_WDM1_REMOVE_BUFFER:
		if( Buffer!=NULL)
		{
			ExFreePool(Buffer);
			Buffer = NULL;
			BufferSize = 0;
		}
		break;

	///////	Get Buffer Size as ULONG
	case IOCTL_WDM1_GET_BUFFER_SIZE:
		if( OutputLength<sizeof(ULONG))
			status = STATUS_INVALID_PARAMETER;
		else
		{
			BytesTxd = sizeof(ULONG);
			RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,&BufferSize,sizeof(ULONG));
		}
		break;

	///////	Get Buffer
	case IOCTL_WDM1_GET_BUFFER:
		if( OutputLength>BufferSize)
			status = STATUS_INVALID_PARAMETER;
		else
		{
			BytesTxd = OutputLength;
			RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer,Buffer,BytesTxd);
		}
		break;

	///////	Invalid request
	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
	}
	// Release shared buffer
	KeReleaseSpinLock(&BufferLock,irql);

	DebugPrint("DeviceIoControl: %d bytes written",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	Wdm2SystemControl:
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

NTSTATUS Wdm2SystemControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
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
//	Wdm2Cleanup:
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

//	Not needed for Wdm2

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

NTSTATUS PowerUpDevice( IN PDEVICE_OBJECT fdo)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;

	// If need be, increase power
	if( dx->PowerState>PowerDeviceD0)
	{
		NTSTATUS status = SendDeviceSetPower( dx, PowerDeviceD0);
		if (!NT_SUCCESS(status))
			return status;
	}

	// Zero our idle counter
	if( dx->PowerIdleCounter)
		PoSetDeviceBusy(dx->PowerIdleCounter);

	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
