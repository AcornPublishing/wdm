//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	WdmIo example
/////////////////////////////////////////////////////////////////////////////
//	dispatch.cpp:		I/O IRP handlers
/////////////////////////////////////////////////////////////////////////////
//	WdmIoCreate			Handle Create/Open file IRP
//	WdmIoClose			Handle Close file IRPs
//	WdmIoRead			Handle Read IRPs
//	WdmIoWrite			Handle Write IRPs
//	WdmIoDeviceControl	Handle DeviceIoControl IRPs
//	WdmIoSystemControl	Handle WMI IRPs
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	14-May-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "wdmIo.h"
#include "Ioctl.h"

/////////////////////////////////////////////////////////////////////////////

NTSTATUS PowerUpDevice( IN PDEVICE_OBJECT fdo);

/////////////////////////////////////////////////////////////////////////////
//	WdmIoCreate:
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

NTSTATUS WdmIoCreate(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DebugPrint( "Create File is %T", &(IrpStack->FileObject->FileName));

	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED);

	InterlockedIncrement(&dx->OpenHandleCount);

	// Complete successfully
	return CompleteIrp(Irp,STATUS_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////
//	WdmIoClose:
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

NTSTATUS WdmIoClose(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("Close");

	// Disconnect from interrupt
	if( dx->ConnectedToInterrupt)
	{
		IoDisconnectInterrupt( dx->InterruptObject);
		dx->ConnectedToInterrupt = false;
	}

	InterlockedDecrement(&dx->OpenHandleCount);

	// Complete successfully
	return CompleteIrp(Irp,STATUS_SUCCESS);
}
 
/////////////////////////////////////////////////////////////////////////////
//	WdmIoRead:
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

NTSTATUS WdmIoRead(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( !dx->GotPortOrMemory)
		return CompleteIrp( Irp, STATUS_INSUFFICIENT_RESOURCES);
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED);
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
		IoStartPacket( fdo, Irp, 0, WdmIoCancelIrp);
		return STATUS_PENDING;
	}
	DebugPrint("Read: %d bytes returned",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	WdmIoWrite:
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

NTSTATUS WdmIoWrite(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( !dx->GotPortOrMemory)
		return CompleteIrp( Irp, STATUS_INSUFFICIENT_RESOURCES);
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED);
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
		IoStartPacket( fdo, Irp, 0, WdmIoCancelIrp);
		return STATUS_PENDING;
	}

	DebugPrint("Write: %d bytes written",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	WdmIoDeviceControl:
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

NTSTATUS WdmIoDeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( !dx->GotPortOrMemory)
		return CompleteIrp( Irp, STATUS_INSUFFICIENT_RESOURCES);
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED);
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
		IoStartPacket( fdo, Irp, 0, WdmIoCancelIrp);
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
//	WdmIoSystemControl:
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

NTSTATUS WdmIoSystemControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
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
//	CompleteIrp:	Sets IoStatus and completes the IRP

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info/*=0*/, IN CCHAR PriorityBoost/*=IO_NO_INCREMENT*/)
{
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp,PriorityBoost);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
