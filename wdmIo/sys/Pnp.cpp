//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	WdmIo example
/////////////////////////////////////////////////////////////////////////////
//	pnp.cpp:		Plug and Play IRP handlers
/////////////////////////////////////////////////////////////////////////////
//	WdmIoAddDevice					Add device routine
//	WdmIoPnp							PNP IRP dispatcher
//	PnpStartDeviceHandler			Handle PnP start device
//*	PnpQueryRemoveDeviceHandler		Handle PnP query remove device
//*	PnpSurpriseRemovalHandler		Handle PnP surprise removal
//*	PnpRemoveDeviceHandler			Handle PnP remove device
//*	PnpStopDeviceHandler			Handle PnP stop device
//*	PnpStopDevice					Handle PnP device stopping
//*	PnpDefaultHandler				Default PnP handler to pass to next stack device
//*	ForwardIrpAndWait				Forward IRP and wait for it to complete
//*	ForwardedIrpCompletionRoutine	Completion routine for ForwardIrpAndWait
//*	LockDevice						Lock out PnP remove request
//*	UnlockDevice					Unlock device allow PnP remove request
//	WdmIoPower						POWER IRP dispatcher
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	19-May-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#define INITGUID // initialize WDMIO_GUID in this module

#include "wdmIo.h"

#pragma code_seg("PAGE")	// start PAGE section

/////////////////////////////////////////////////////////////////////////////

NTSTATUS PnpStartDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpQueryRemoveDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpSurpriseRemovalHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpRemoveDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS PnpStopDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
void PnpStopDevice( IN PWDMIO_DEVICE_EXTENSION dx);
NTSTATUS PnpDefaultHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp);

NTSTATUS ForwardIrpAndWait( IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS ForwardedIrpCompletionRoutine( IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PKEVENT ev);

/////////////////////////////////////////////////////////////////////////////
//	WdmIoAddDevice:
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

NTSTATUS WdmIoAddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo)
{
	DebugPrint("AddDevice");
	NTSTATUS status;
	PDEVICE_OBJECT fdo;

	// Create our Functional Device Object in fdo
	status = IoCreateDevice( DriverObject, sizeof(WDMIO_DEVICE_EXTENSION), NULL,	// No Name
								FILE_DEVICE_UNKNOWN, 0, FALSE, &fdo);
	if( !NT_SUCCESS(status))
		return status;

	// Remember fdo in our device extension
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	dx->fdo = fdo;
	dx->pdo = pdo;
	dx->UsageCount = 1;
	KeInitializeEvent( &dx->StoppingEvent, NotificationEvent, FALSE);
	dx->OpenHandleCount = 0;
	dx->GotResources = false;
	dx->Paused = false;
	dx->IODisabled = true;
	dx->Stopping = false;
	dx->GotPortOrMemory = false;
	dx->GotInterrupt = false;
	dx->ConnectedToInterrupt = false;
	dx->SetTimeout = 10;
	dx->Timeout = -1;
	dx->StopTimer = false;
	dx->WriteCmds = NULL;
	dx->ReadCmds = NULL;
	dx->StartReadCmds = NULL;
	DebugPrint("FDO is %x",fdo);

	// Initialise "connect to interrupt" work queue item
	ExInitializeWorkItem( &dx->ConnectIntWQI, IrqConnectRoutine, dx);
	dx->ConnectIntQueued = false;

	// Initialise timer for this device (but do not start)
	status = IoInitializeTimer( fdo, (PIO_TIMER_ROUTINE)Timeout1s, dx);
	if( !NT_SUCCESS(status))
	{
		IoDeleteDevice(fdo);
		return status;
	}

	// Initialise our DPC for IRQ completion processing
	IoInitializeDpcRequest( fdo, WdmIoDpcForIsr);

	// Register and enable our device interface
	status = IoRegisterDeviceInterface( pdo, &WDMIO_GUID, NULL, &dx->ifSymLinkName);
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
//	WdmIoPnp:
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

NTSTATUS WdmIoPnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx=(PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
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
#if DBG
	if( status!=STATUS_SUCCESS)
		DebugPrint("PnP completed %x",status);
#endif
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	PnpStartDeviceHandler:	Handle PnP start device

void ListResources( IN PCM_RESOURCE_LIST AllocatedResources)
{
	if( AllocatedResources==NULL ||
		AllocatedResources->Count==0)
	{
		DebugPrintMsg("ListResources: No allocated resources");
		return;
	}

	// Get to actual resources
	PCM_PARTIAL_RESOURCE_LIST list = &AllocatedResources->List[0].PartialResourceList;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = list->PartialDescriptors;
	ULONG NumResources = list->Count;

	DebugPrint("ListResources: %d resource lists %d resources", AllocatedResources->Count, NumResources);

	bool GotError = false;

	// Go through each allocated resource
	for( ULONG resno=0; resno<NumResources; resno++,resource++)
	{
		switch( resource->Type)
		{
		case CmResourceTypePort:
		{
			PHYSICAL_ADDRESS PortStartAddress = resource->u.Port.Start;
			ULONG PortLength = resource->u.Port.Length;
			bool PortNeedsMapping = (resource->Flags & CM_RESOURCE_PORT_IO)==0;
			bool PortInIOSpace = !PortNeedsMapping;
			DebugPrint("ListResources: Port %L Length %d NeedsMapping %d",
							PortStartAddress,
							PortLength, PortNeedsMapping);
		}
			break;

		case CmResourceTypeInterrupt:
		{
			KIRQL Irql = (KIRQL)resource->u.Interrupt.Level;
			ULONG Vector = resource->u.Interrupt.Vector;
			KAFFINITY Affinity = resource->u.Interrupt.Affinity;
			KINTERRUPT_MODE Mode = (resource->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
						? Latched : LevelSensitive;
			DebugPrint("ListResources: Interrupt vector %x IRQL %d Affinity %d Mode %d",
							Vector, Irql, Affinity, Mode);
			break;
		}

		case CmResourceTypeMemory:
		{
			PHYSICAL_ADDRESS PortStartAddress = resource->u.Memory.Start;
			ULONG PortLength = resource->u.Memory.Length;
			bool PortInIOSpace = false;
			bool PortNeedsMapping = true;
			DebugPrint("ListResources: Memory %L Length %d",
							PortStartAddress, PortLength);
			break;
		}

		case CmResourceTypeDma:
		case CmResourceTypeDeviceSpecific:
		case CmResourceTypeBusNumber:
		default:
			DebugPrint("ListResources: Unrecognised resource type %d", resource->Type);
			GotError = true;
			break;
		}
	}
}

NTSTATUS PnpStartDeviceHandler( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	DebugPrintMsg("PnpStartDeviceHandler");
	PWDMIO_DEVICE_EXTENSION dx=(PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);

	NTSTATUS status = ForwardIrpAndWait( fdo, Irp);
	if( !NT_SUCCESS(status))
		return CompleteIrp( Irp, status, Irp->IoStatus.Information);
	
	DebugPrint("PnpStartDeviceHandler: post-processing");
ListResources(IrpStack->Parameters.StartDevice.AllocatedResources);
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
	PWDMIO_DEVICE_EXTENSION dx=(PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
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
	PWDMIO_DEVICE_EXTENSION dx=(PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
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
	PWDMIO_DEVICE_EXTENSION dx=(PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("PnpRemoveDeviceHandler");

	// Wait for I/O to complete and stop device
	PnpStopDevice(dx);

	// Pass down stack and carry on immediately
	NTSTATUS status = PnpDefaultHandler(fdo, Irp);

	// Stop timer (not sure if this is necessary)
	if( dx->StopTimer)
		IoStopTimer(fdo);

	// Remove any stored command buffers
	FreeIfAllocated(dx->WriteCmds);
	FreeIfAllocated(dx->StartReadCmds);
	FreeIfAllocated(dx->ReadCmds);

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
	PWDMIO_DEVICE_EXTENSION dx=(PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;

	// Wait for I/O to complete and stop device
	PnpStopDevice(dx);
	
	return PnpDefaultHandler( fdo, Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	PnpStopDevice:	Handle PnP device stopping

void PnpStopDevice( IN PWDMIO_DEVICE_EXTENSION dx)
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
	DebugPrintMsg("PnpDefaultHandler");
	PWDMIO_DEVICE_EXTENSION dx=(PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	IoSkipCurrentIrpStackLocation(Irp);
	return IoCallDriver( dx->NextStackDevice, Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	ForwardIrpAndWait:	Forward IRP and wait for it to complete

NTSTATUS ForwardIrpAndWait( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	DebugPrintMsg("ForwardIrpAndWait");
	PWDMIO_DEVICE_EXTENSION dx=(PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;

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

bool LockDevice( IN PWDMIO_DEVICE_EXTENSION dx)
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

void UnlockDevice( IN PWDMIO_DEVICE_EXTENSION dx)
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

#pragma code_seg("PAGE")	// start PAGE section

/////////////////////////////////////////////////////////////////////////////
//	WdmIoPower:
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

NTSTATUS WdmIoPower(	IN PDEVICE_OBJECT fdo,
						IN PIRP Irp)
{
	DebugPrint("Power %I",Irp);
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;

	// Just pass to lower driver
	PoStartNextPowerIrp( Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver( dx->NextStackDevice, Irp);
}

#pragma code_seg()	// end PAGE section
