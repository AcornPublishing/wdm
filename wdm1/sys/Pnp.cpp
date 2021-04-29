//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm1 example
/////////////////////////////////////////////////////////////////////////////
//	pnp.cpp:		Plug and Play and Power IRP handlers
/////////////////////////////////////////////////////////////////////////////
//	Wdm1AddDevice	Add device routine
//	Wdm1Pnp			PNP IRP dispatcher
//	Wdm1Power		POWER IRP dispatcher
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#define INITGUID		// initialize WDM1_GUID in this module
 
#include "wdm1.h"

#pragma code_seg("PAGE")	// start PAGE section

/////////////////////////////////////////////////////////////////////////////
//	Wdm1AddDevice:
//
//	Description:
//		Cope with a new Pnp device being added here.
//		Usually just attach to the top of the driver stack.
//		Do not talk to device here!
//
//	Arguments:
//		Pointer to the Driver object
//		Pointer to Physical Device Object
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS Wdm1AddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo)
{
	DebugPrint("AddDevice");
	NTSTATUS status;
	PDEVICE_OBJECT fdo;

	// Create our Functional Device Object in fdo
	status = IoCreateDevice (DriverObject,
		sizeof(WDM1_DEVICE_EXTENSION),
		NULL,	// No Name
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,	// Not exclusive
		&fdo);
	if( !NT_SUCCESS(status))
		return status;

	// Remember fdo in our device extension
	PWDM1_DEVICE_EXTENSION dx = (PWDM1_DEVICE_EXTENSION)fdo->DeviceExtension;
	dx->fdo = fdo;
	DebugPrint("FDO is %x",fdo);

	// Register and enable our device interface
	status = IoRegisterDeviceInterface(pdo, &WDM1_GUID, NULL, &dx->ifSymLinkName);
	if( !NT_SUCCESS(status))
	{
		IoDeleteDevice(fdo);
		return status;
	}
	IoSetDeviceInterfaceState(&dx->ifSymLinkName, TRUE);
	DebugPrint("Symbolic Link Name is %T",&dx->ifSymLinkName);

	// Attach to the driver stack below us
	dx->NextStackDevice = IoAttachDeviceToDeviceStack(fdo,pdo);

	// Set fdo flags appropriately
	fdo->Flags |= DO_BUFFERED_IO|DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//	Wdm1Pnp:
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

NTSTATUS Wdm1Pnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	DebugPrint("PnP %I",Irp);
	PWDM1_DEVICE_EXTENSION dx=(PWDM1_DEVICE_EXTENSION)fdo->DeviceExtension;

	// Remember minor function
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG MinorFunction = IrpStack->MinorFunction;

	// Just pass to lower driver
	IoSkipCurrentIrpStackLocation(Irp);
	NTSTATUS status = IoCallDriver( dx->NextStackDevice, Irp);

	// Device removed
	if( MinorFunction==IRP_MN_REMOVE_DEVICE)
	{
		DebugPrint("PnP RemoveDevice");
		// disable device interface
		IoSetDeviceInterfaceState(&dx->ifSymLinkName, FALSE);
		RtlFreeUnicodeString(&dx->ifSymLinkName);
		
		// unattach from stack
		if (dx->NextStackDevice)
			IoDetachDevice(dx->NextStackDevice);

		// delete our fdo
		IoDeleteDevice(fdo);
	}

	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	Wdm1Power:
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

NTSTATUS Wdm1Power(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	DebugPrint("Power %I",Irp);
	PWDM1_DEVICE_EXTENSION dx = (PWDM1_DEVICE_EXTENSION)fdo->DeviceExtension;

	// Just pass to lower driver
	PoStartNextPowerIrp( Irp);
	IoSkipCurrentIrpStackLocation(Irp);
	return PoCallDriver( dx->NextStackDevice, Irp);
}

#pragma code_seg()	// end PAGE section
