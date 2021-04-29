//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	HidKbd example
/////////////////////////////////////////////////////////////////////////////
//	dispatch.cpp:		I/O IRP handlers
/////////////////////////////////////////////////////////////////////////////
//	HidKbdCreate		Handle Create/Open file IRP
//	HidKbdClose			Handle Close file IRPs
//	HidKbdRead			Handle Read IRPs
//	HidKbdWrite			Handle Write IRPs
//	HidKbdDeviceControl	Handle DeviceIoControl IRPs
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "HidKbd.h"

/////////////////////////////////////////////////////////////////////////////
//	HidKbdCreate:
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

NTSTATUS HidKbdCreate(	IN PDEVICE_OBJECT fdo,
						IN PIRP Irp)
{
	PHIDKBD_DEVICE_EXTENSION dx = (PHIDKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	DebugPrint( "Create File is %T", &(IrpStack->FileObject->FileName));

	InterlockedIncrement(&dx->OpenHandleCount);

	// Forward IRP to HID class driver device
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver( dx->HidDevice, Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	HidKbdClose:
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

NTSTATUS HidKbdClose(	IN PDEVICE_OBJECT fdo,
						IN PIRP Irp)
{
	PHIDKBD_DEVICE_EXTENSION dx = (PHIDKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("Close");

	InterlockedDecrement(&dx->OpenHandleCount);

	// Forward IRP to HID class driver device
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver( dx->HidDevice, Irp);
}
 
/////////////////////////////////////////////////////////////////////////////
//	HidKbdRead:
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

NTSTATUS HidKbdRead(	IN PDEVICE_OBJECT fdo,
						IN PIRP Irp)
{
	PHIDKBD_DEVICE_EXTENSION dx = (PHIDKBD_DEVICE_EXTENSION)fdo->DeviceExtension;

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG BytesTxd = 0;
	NTSTATUS status = STATUS_SUCCESS;

	// Get call parameters
	LONGLONG FilePointer = IrpStack->Parameters.Read.ByteOffset.QuadPart;
	ULONG ReadLen = IrpStack->Parameters.Read.Length;
	DebugPrint("Read %d bytes from file pointer %d",(int)ReadLen,(int)FilePointer);

	if( ReadLen>=dx->HidInputReportLen)
	{
		status = ReadHidKbdInputReport( dx, IrpStack->FileObject, Irp->AssociatedIrp.SystemBuffer, BytesTxd);
	}

	DebugPrint("Read: %d bytes returned",(int)BytesTxd);

	// Complete IRP
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = BytesTxd;
	if( NT_SUCCESS(status))
		IoCompleteRequest(Irp,IO_KEYBOARD_INCREMENT);
	else
		IoCompleteRequest(Irp,IO_NO_INCREMENT);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	HidKbdWrite:
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

NTSTATUS HidKbdWrite(	IN PDEVICE_OBJECT fdo,
						IN PIRP Irp)
{
	PHIDKBD_DEVICE_EXTENSION dx = (PHIDKBD_DEVICE_EXTENSION)fdo->DeviceExtension;

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG BytesTxd = 0;
	NTSTATUS status = STATUS_SUCCESS;

	// Get call parameters
	LONGLONG FilePointer = IrpStack->Parameters.Write.ByteOffset.QuadPart;
	ULONG WriteLen = IrpStack->Parameters.Write.Length;
	DebugPrint("Write %d bytes from file pointer %d",(int)WriteLen,(int)FilePointer);

	if( FilePointer<0)
		status = STATUS_INVALID_PARAMETER;

	DebugPrint("Write: %d bytes written",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	HidKbdDeviceControl:
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

NTSTATUS HidKbdDeviceControl(	IN PDEVICE_OBJECT fdo,
								IN PIRP Irp)
{
	PHIDKBD_DEVICE_EXTENSION dx = (PHIDKBD_DEVICE_EXTENSION)fdo->DeviceExtension;

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG BytesTxd = 0;
	NTSTATUS status = STATUS_SUCCESS;

	ULONG ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

	DebugPrint("DeviceIoControl: Control code %x InputLength %d OutputLength %d",
				ControlCode, InputLength, OutputLength);

	DebugPrint("DeviceIoControl: %d bytes written",(int)BytesTxd);

	// Complete IRP
	CompleteIrp(Irp,status,BytesTxd);
	return status;
}

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
