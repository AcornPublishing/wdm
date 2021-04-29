//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm2 example
/////////////////////////////////////////////////////////////////////////////
//	power.cpp:		Power IRP handler
/////////////////////////////////////////////////////////////////////////////
//	Wdm2Power						Power IRP dispatcher
//*	PowerSetPower					Handle set system and device power
//*	DefaultPowerHandler				Pass Power IRP down stack
//*	OnCompleteIncreaseSystemPower	Set system power (increase device power) completed
//*	OnCompleteIncreaseDevicePower	Set device power completed
//	SendDeviceSetPower				Send set power for device
//*	OnCompleteDeviceSetPower		Set device power IRP completed
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "wdm2.h"

/////////////////////////////////////////////////////////////////////////////

NTSTATUS PowerSetPower( IN PWDM2_DEVICE_EXTENSION dx, IN PIRP Irp);
NTSTATUS DefaultPowerHandler( IN PWDM2_DEVICE_EXTENSION dx, IN PIRP Irp);
NTSTATUS OnCompleteIncreaseSystemPower( IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PVOID context);
NTSTATUS OnCompleteIncreaseDevicePower( IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PVOID context);
VOID OnCompleteDeviceSetPower( IN PDEVICE_OBJECT fdo, IN UCHAR MinorFunction,
					IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus);

/////////////////////////////////////////////////////////////////////////////
//	Wdm2Power:
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

NTSTATUS Wdm2Power(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
	if( dx->IODisabled)
		return CompleteIrp( Irp, STATUS_DEVICE_NOT_CONNECTED, 0);
	if (!LockDevice(dx))
		return CompleteIrp( Irp, STATUS_DELETE_PENDING, 0);

	NTSTATUS status = STATUS_SUCCESS;
	DebugPrint("Power %I",Irp);

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG MinorFunction = IrpStack->MinorFunction;

	if( MinorFunction==IRP_MN_SET_POWER)
		status = PowerSetPower(dx,Irp);
	else
		status = DefaultPowerHandler(dx,Irp);

	UnlockDevice(dx);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	PowerSetPower:	Handle set system and device power

NTSTATUS PowerSetPower( IN PWDM2_DEVICE_EXTENSION dx, IN PIRP Irp)
{
	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	POWER_STATE_TYPE PowerType = IrpStack->Parameters.Power.Type;
	POWER_STATE PowerState = IrpStack->Parameters.Power.State;

	/////////////////////////////////////////////////////////////////////////
	//	Set System Power

	if( PowerType==SystemPowerState)
	{
		DEVICE_POWER_STATE DesiredDevicePowerState =
			(PowerState.SystemState<=PowerSystemWorking ? PowerDeviceD0 : PowerDeviceD3);
		
		if( DesiredDevicePowerState<dx->PowerState)
		{
			// This system state means we have to increase device power
			DebugPrint("System state %d.  Increase device power to %d",
						PowerState.SystemState, DesiredDevicePowerState);
			// Process on way up stack...
			PoStartNextPowerIrp(Irp);
			IoCopyCurrentIrpStackLocationToNext(Irp);
			IoSetCompletionRoutine( Irp, OnCompleteIncreaseSystemPower, NULL, TRUE, TRUE, TRUE);
			return PoCallDriver( dx->NextStackDevice, Irp);
		}
		else if( DesiredDevicePowerState>dx->PowerState)
		{
			// This system state means we have to decrease device power
			DebugPrint("System state %d.  Decrease device power to %d",
						PowerState.SystemState, DesiredDevicePowerState);
			// Send power down request to device
			status = SendDeviceSetPower( dx, DesiredDevicePowerState);
			if( !NT_SUCCESS(status))
			{
				PoStartNextPowerIrp(Irp);
				return CompleteIrp( Irp, status, 0);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////
	//	Set Device Power
	else if( PowerType==DevicePowerState)
	{
		DEVICE_POWER_STATE DesiredDevicePowerState = PowerState.DeviceState;

		if( DesiredDevicePowerState<dx->PowerState)
		{
			// Increase device power state
			DebugPrint("Increase device power to %d", DesiredDevicePowerState);
			// Process on way up stack...
			PoStartNextPowerIrp(Irp);
			IoCopyCurrentIrpStackLocationToNext(Irp);
			IoSetCompletionRoutine( Irp, OnCompleteIncreaseDevicePower, NULL, TRUE, TRUE, TRUE);
			return PoCallDriver( dx->NextStackDevice, Irp);
		}
		else if( DesiredDevicePowerState>dx->PowerState)
		{
			// Decrease device power state
			DebugPrint("Decrease device power to %d", DesiredDevicePowerState);
			// Set power state
			SetPowerState(dx,PowerState.DeviceState);
		}
	}

	/////////////////////////////////////////////////////////////////////////
	//	Unrecognised Set Power
#if DBG
	else
		DebugPrint("Power: unrecognised power type %d",PowerType);
#endif

	// Finally pass to lower drivers
	return DefaultPowerHandler(dx,Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	DefaultPowerHandler:	Pass Power IRP down stack

NTSTATUS DefaultPowerHandler( IN PWDM2_DEVICE_EXTENSION dx, IN PIRP Irp)
{
	DebugPrintMsg("DefaultPowerHandler");
	// Just pass to lower driver
	PoStartNextPowerIrp( Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver( dx->NextStackDevice, Irp);
}

/////////////////////////////////////////////////////////////////////////////
//	OnCompleteIncreaseSystemPower:	Set system power (increase device power) completed

NTSTATUS OnCompleteIncreaseSystemPower( IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PVOID context)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);
	NTSTATUS status = Irp->IoStatus.Status;
	DebugPrint("OnCompleteIncreaseSystemPower %x",status);
	if( !NT_SUCCESS(status))
		return status;

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	POWER_STATE PowerState = IrpStack->Parameters.Power.State;
	DEVICE_POWER_STATE DesiredDevicePowerState =
		(PowerState.SystemState<=PowerSystemWorking ? PowerDeviceD0 : PowerDeviceD3);
	if( DesiredDevicePowerState<dx->PowerState)
		status = SendDeviceSetPower( dx, DesiredDevicePowerState);

	PoStartNextPowerIrp(Irp);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	OnCompleteIncreaseDevicePower:	Set device power completed

NTSTATUS OnCompleteIncreaseDevicePower( IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PVOID context)
{
	PWDM2_DEVICE_EXTENSION dx = (PWDM2_DEVICE_EXTENSION)fdo->DeviceExtension;
	if (Irp->PendingReturned)
		IoMarkIrpPending(Irp);
	NTSTATUS status = Irp->IoStatus.Status;
	DebugPrint("OnCompleteIncreaseDevicePower %x",status);
	if( !NT_SUCCESS(status))
		return status;

	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	POWER_STATE PowerState = IrpStack->Parameters.Power.State;
	SetPowerState(dx,PowerState.DeviceState);

	PoStartNextPowerIrp(Irp);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	SendDeviceSetPower:	Send set power for device
//						Must not be called from set device power as it waits on an event

typedef struct _SDSP
{
	KEVENT event;
	NTSTATUS Status;
} SDSP, *PSDSP;

NTSTATUS SendDeviceSetPower( IN PWDM2_DEVICE_EXTENSION dx, IN DEVICE_POWER_STATE NewDevicePowerState)
{
	DebugPrint("SendDeviceSetPower to %d", NewDevicePowerState);
	POWER_STATE NewState;
	NewState.DeviceState = NewDevicePowerState;
	SDSP sdsp;
	KeInitializeEvent( &sdsp.event, NotificationEvent, FALSE);
	sdsp.Status = STATUS_SUCCESS;
	NTSTATUS status = PoRequestPowerIrp( dx->pdo, IRP_MN_SET_POWER, NewState,
		OnCompleteDeviceSetPower, &sdsp, NULL);
	if( status==STATUS_PENDING)
	{
		KeWaitForSingleObject( &sdsp.event, Executive, KernelMode, FALSE, NULL);
		status = sdsp.Status;
	}

	// Cope with W98 not passing power irp to us
	if( NT_SUCCESS(status) && dx->PowerState!=NewDevicePowerState)
	{
		DebugPrintMsg("SendDeviceSetPower: Device state not set properly by us.  Setting again");
		SetPowerState(dx,NewDevicePowerState);
	}

	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	OnCompleteDeviceSetPower:	Set device power IRP completed

VOID OnCompleteDeviceSetPower( IN PDEVICE_OBJECT fdo, IN UCHAR MinorFunction,
					IN POWER_STATE PowerState, IN PVOID Context, IN PIO_STATUS_BLOCK IoStatus)
{
	DebugPrintMsg("OnCompleteDeviceSetPower");
	PSDSP psdsp = (PSDSP)Context;
	psdsp->Status = IoStatus->Status;
	KeSetEvent( &psdsp->event, 0, FALSE);
}

/////////////////////////////////////////////////////////////////////////////
