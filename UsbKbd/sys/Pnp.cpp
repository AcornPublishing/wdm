//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	UsbKbd example
/////////////////////////////////////////////////////////////////////////////
//	pnp.cpp:		Plug and Play IRP handlers
/////////////////////////////////////////////////////////////////////////////
//	UsbKbdAddDevice					Add device routine
//	UsbKbdPnp						PNP IRP dispatcher
//	PnpStartDeviceHandler			Handle PnP start device
//*	PnpQueryCapabilitiesHandler		Print pdo device capabilities
//*	PnpQueryRemoveDeviceHandler		Handle PnP query remove device
//*	PnpSurpriseRemovalHandler		Handle PnP surprise removal
//*	PnpRemoveDeviceHandler			Handle PnP remove device
//*	PnpStopDeviceHandler			Handle PnP stop device
//*	PnpStopDevice					Handle PnP device stopping
//*	PnpDefaultHandler				Default PnP handler to pass to next stack device
//*	ForwardIrpAndWait				Forward IRP and wait for it to complete
//*	LockDevice						Lock out PnP remove request
//*	UnlockDevice					Unlock device allow PnP remove request
//	UsbKbdPower						Power IRP handler
//*	ForwardedIrpCompletionRoutine	Completion routine for ForwardIrpAndWait
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	19-May-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#define INITGUID // initialize USBKBD_GUID in this module

#include "UsbKbd.h"

#pragma code_seg("PAGE")	// start PAGE section

/////////////////////////////////////////////////////////////////////////////

NTSTATUS PnpStartDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpQueryCapabilitiesHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpQueryRemoveDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpSurpriseRemovalHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpRemoveDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpStopDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
void PnpStopDevice( IN PUSBKBD_DEVICE_EXTENSION dx);
NTSTATUS PnpDefaultHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);

NTSTATUS ForwardedIrpCompletionRoutine( IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PKEVENT ev);

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdAddDevice:
//
//	Description:
//		Cope with a new PnP device being added here.
//		Usually just attach to the top of the driver stack.
//		Do not talk to device here!
//
//	Arguments:
//		Pointer to the Driver object
//		Pointer to Physical Device Object
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdAddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo)
{
	DebugPrint("AddDevice");
	NTSTATUS status;
	PDEVICE_OBJECT fdo;

	// Create our Functional Device Object in fdo
	status = IoCreateDevice( DriverObject, sizeof(USBKBD_DEVICE_EXTENSION), NULL,	// No Name
								FILE_DEVICE_UNKNOWN, 0, FALSE, &fdo);
	if( !NT_SUCCESS(status))
		return status;

	// Remember fdo in our device extension
	PUSBKBD_DEVICE_EXTENSION dx = (PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	dx->fdo = fdo;
	dx->pdo = pdo;
	dx->UsageCount = 1;
	KeInitializeEvent( &dx->StoppingEvent, NotificationEvent, FALSE);
	dx->OpenHandleCount = 0;
	dx->GotResources = false;
	dx->Paused = false;
	dx->IODisabled = true;
	dx->Stopping = false;
	dx->UsbConfigurationHandle = NULL;
	dx->UsbPipeHandle = NULL;
	dx->UsbTimeout = 10;
	DebugPrint("FDO is %x",fdo);

	// Register and enable our device interface
	status = IoRegisterDeviceInterface( pdo, &USBKBD_GUID, NULL, &dx->ifSymLinkName);
	if( !NT_SUCCESS(status))
	{
		IoDeleteDevice(fdo);
		return status;
	}
	DebugPrint("Symbolic Link Name is %T",&dx->ifSymLinkName);

	// Attach to the driver stack below us
	dx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo,pdo);

	// Set fdo flags appropriately
	fdo->Flags |= DO_BUFFERED_IO|DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdPnp:
//
//	Description:
//		Handle IRP_MJ_PNP requests
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			Various minor codes
//				IrpStack->Parameters.QueryDeviceRelations
//				IrpStack->Parameters.QueryInterface
//				IrpStack->Parameters.DeviceCapabilities
//				IrpStack->Parameters.FilterResourceRequirements
//				IrpStack->Parameters.ReadWriteConfig
//				IrpStack->Parameters.SetLock
//				IrpStack->Parameters.QueryId
//				IrpStack->Parameters.QueryDeviceText
//				IrpStack->Parameters.UsageNotification
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdPnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx=(PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrint("PnP %I",Irp);

	if (!LockDevice(dx))
		return CompleteIrp(Irp, STATUS_DELETE_PENDING, 0);

	// Get minor function
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG MinorFunction = IrpStack->MinorFunction;

	NTSTATUS status = STATUS_SUCCESS;
	switch( MinorFunction)
	{
	case IRP_MN_START_DEVICE:
		status = PnpStartDeviceHandler(fdo,Irp);
		break;
	case IRP_MN_QUERY_REMOVE_DEVICE:
		status = PnpQueryRemoveDeviceHandler(fdo,Irp);
		break;
	case IRP_MN_SURPRISE_REMOVAL:
		status = PnpSurpriseRemovalHandler(fdo,Irp);
		break;
	case IRP_MN_REMOVE_DEVICE:
		status = PnpRemoveDeviceHandler(fdo,Irp);
		return status;
	case IRP_MN_QUERY_STOP_DEVICE:
		dx->Paused = true;
		dx->IODisabled = true;
		status = PnpDefaultHandler(fdo,Irp);
		break;
	case IRP_MN_STOP_DEVICE:
		status = PnpStopDeviceHandler(fdo,Irp);
		break;
	case IRP_MN_CANCEL_REMOVE_DEVICE:	// fall thru
	case IRP_MN_CANCEL_STOP_DEVICE:
		dx->Paused = false;
		dx->IODisabled = false;
		// fall thru
	default:
		status = PnpDefaultHandler(fdo,Irp);
	}

	UnlockDevice(dx);
//#if DBG
//	if( status!=STATUS_SUCCESS)
//		DebugPrint("PnP completed %x",status);
//#endif
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	PnpStartDeviceHandler:	Handle PnP start device

NTSTATUS PnpStartDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	DebugPrintMsg("PnpStartDeviceHandler");
	PUSBKBD_DEVICE_EXTENSION dx=(PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

	NTSTATUS status = ForwardIrpAndWait( fdo, Irp);
	if( !NT_SUCCESS(status))
		return CompleteIrp( Irp, status, Irp->IoStatus.Information);
	
	DebugPrint("PnpStartDeviceHandler: post-processing");
	status = StartDevice( dx, IrpStack->Parameters.StartDevice.AllocatedResourcesTranslated);
	if( NT_SUCCESS(status))
	{
		dx->Paused = false;
		dx->IODisabled = false;
		IoSetDeviceInterfaceState( &dx->ifSymLinkName, TRUE);
	}

	return CompleteIrp( Irp, status, 0);
}


/////////////////////////////////////////////////////////////////////////////
//	PnpQueryRemoveDeviceHandler:	Handle PnP query remove device

NTSTATUS PnpQueryRemoveDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx=(PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("PnpQueryRemoveDeviceHandler");
	if( dx->OpenHandleCount>0)
	{
		DebugPrint("PnpQueryRemoveDeviceHandler: %d handles still open",dx->OpenHandleCount);
		return CompleteIrp( Irp, STATUS_UNSUCCESSFUL, 0);
	}
	dx->Paused = true;
	dx->IODisabled = true;
	return PnpDefaultHandler(fdo,Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	PnpSurpriseRemovalHandler:	Handle PnP surprise removal

NTSTATUS PnpSurpriseRemovalHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx=(PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("PnpSurpriseRemovalHandler");

	// Wait for I/O to complete and stop device
	PnpStopDevice(dx);

	// Pass down stack and carry on immediately
	return PnpDefaultHandler(fdo, Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	PnpRemoveDeviceHandler:	Handle PnP remove device

NTSTATUS PnpRemoveDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PUSBKBD_DEVICE_EXTENSION dx=(PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("PnpRemoveDeviceHandler");

	// Wait for I/O to complete and stop device
	PnpStopDevice(dx);

	// Pass down stack and carry on immediately
	NTSTATUS status = PnpDefaultHandler(fdo, Irp);
	
	// disable device interface
	IoSetDeviceInterfaceState( &dx->ifSymLinkName, FALSE);
	RtlFreeUnicodeString(&dx->ifSymLinkName);
	
	// unattach from stack
	if (dx->NextStackDevice)
		IoDetachDevice(dx->NextStackDevice);

	// delete our fdo
	IoDeleteDevice(fdo);

	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	PnpStopDeviceHandler:	Handle PnP stop device

NTSTATUS PnpStopDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	DebugPrintMsg("PnpStopDeviceHandler");
	PUSBKBD_DEVICE_EXTENSION dx=(PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;

	// Wait for I/O to complete and stop device
	PnpStopDevice(dx);
	
	return PnpDefaultHandler( fdo, Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	PnpStopDevice:	Handle PnP device stopping

void PnpStopDevice( IN PUSBKBD_DEVICE_EXTENSION dx)
{
	// Stop I/O ASAP
	dx->IODisabled = true;

	// Do nothing if we're already stopped
	if( !dx->GotResources)
		return;

	// Wait for any pending I/O operations to complete
	dx->Stopping = true;
	KeResetEvent(&dx->StoppingEvent);
	UnlockDevice(dx);
	UnlockDevice(dx);
	KeWaitForSingleObject( &dx->StoppingEvent, Executive, KernelMode, FALSE, NULL);
	DebugPrint("PnpStopDevice: All pending I/O completed");
	dx->Stopping = false;

	// Stop our device before passing down
	StopDevice(dx);

	// Bump usage count back up again
	LockDevice(dx);
	LockDevice(dx);
}

/////////////////////////////////////////////////////////////////////////////
//	Support routines
/////////////////////////////////////////////////////////////////////////////
//	PnpDefaultHandler:	Default PnP handler to pass to next stack device

NTSTATUS PnpDefaultHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
//	DebugPrintMsg("PnpDefaultHandler");
	PUSBKBD_DEVICE_EXTENSION dx=(PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver( dx->NextStackDevice, Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	ForwardIrpAndWait:	Forward IRP and wait for it to complete

NTSTATUS ForwardIrpAndWait( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	DebugPrintMsg("ForwardIrpAndWait");
	PUSBKBD_DEVICE_EXTENSION dx=(PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;

	KEVENT event;
	KeInitializeEvent( &event, NotificationEvent, FALSE);

	IoCopyCurrentIrpStackLocationToNext(Irp);
	IoSetCompletionRoutine( Irp, (PIO_COMPLETION_ROUTINE)ForwardedIrpCompletionRoutine,
								(PVOID)&event, TRUE, TRUE, TRUE);

	NTSTATUS status = IoCallDriver( dx->NextStackDevice, Irp);
	if( status==STATUS_PENDING)
	{
		DebugPrintMsg("ForwardIrpAndWait: waiting for completion");
		KeWaitForSingleObject( &event, Executive, KernelMode, FALSE, NULL);
		status = Irp->IoStatus.Status;
	}
#if DBG
	if( status!=STATUS_SUCCESS)
		DebugPrint("ForwardIrpAndWait: completed %x",status);
#endif
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	LockDevice:	Lock out PnP remove request

bool LockDevice( IN PUSBKBD_DEVICE_EXTENSION dx)
{
//	DebugPrintMsg("LockDevice");
	InterlockedIncrement(&dx->UsageCount);

	if( dx->Stopping)
	{
		if( InterlockedDecrement(&dx->UsageCount)==0)
			KeSetEvent( &dx->StoppingEvent, 0, FALSE);
		return false;
	}
	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	UnlockDevice:	Unlock device allow PnP remove request

void UnlockDevice( IN PUSBKBD_DEVICE_EXTENSION dx)
{
//	DebugPrintMsg("UnlockDevice");
	LONG UsageCount = InterlockedDecrement(&dx->UsageCount);
	if( UsageCount==0)
	{
		DebugPrintMsg("UnlockDevice: setting StoppingEvent flag");
		KeSetEvent( &dx->StoppingEvent, 0, FALSE);
	}
}

/////////////////////////////////////////////////////////////////////////////
//	UsbKbdPower:
//
//	Description:
//		Handle IRP_MJ_POWER requests
//
//	Arguments:
//		Pointer to the FDO
//		Pointer to the IRP
//			IRP_MN_WAIT_WAKE:		IrpStack->Parameters.WaitWake.Xxx
//			IRP_MN_POWER_SEQUENCE:	IrpStack->Parameters.PowerSequence.Xxx
//			IRP_MN_SET_POWER:
//			IRP_MN_QUERY_POWER:		IrpStack->Parameters.Power.Xxx
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS UsbKbdPower(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	DebugPrint("Power %I",Irp);
	PUSBKBD_DEVICE_EXTENSION dx = (PUSBKBD_DEVICE_EXTENSION)fdo->DeviceExtension;

	// Just pass to lower driver
	PoStartNextPowerIrp( Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver( dx->NextStackDevice, Irp);
}

/////////////////////////////////////////////////////////////////////////////

#pragma code_seg()	// end PAGE section

/////////////////////////////////////////////////////////////////////////////
//	ForwardedIrpCompletionRoutine:	Completion routine for ForwardIrpAndWait
//	Can be called at DISPATCH_LEVEL IRQL

NTSTATUS ForwardedIrpCompletionRoutine( IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PKEVENT ev)
{
	KeSetEvent( ev, 0, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

/////////////////////////////////////////////////////////////////////////////

