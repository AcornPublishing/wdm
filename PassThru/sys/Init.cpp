//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	PassThru example
/////////////////////////////////////////////////////////////////////////////
//	init.cpp:		Driver initialization code
/////////////////////////////////////////////////////////////////////////////
//	DriverEntry		Initialisation entry point
//	PassThruUnload		Unload driver routine
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "PassThru.h"

NTSTATUS PassThru(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);


#pragma code_seg("INIT") // start INIT section

/////////////////////////////////////////////////////////////////////////////
//	DriverEntry:
//
//	Description:
//		This function initializes the driver, and creates
//		any objects needed to process I/O requests.
//
//	Arguments:
//		Pointer to the Driver object
//		Registry path string for driver service key
//
//	Return Value:
//		This function returns STATUS_XXX

extern "C"
NTSTATUS DriverEntry(	IN PDRIVER_OBJECT DriverObject,
						IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS status = STATUS_SUCCESS;

#if DBG
	DebugPrintInit("PassThru checked");
#else
	DebugPrintInit("PassThru free");
#endif

	DebugPrint("RegistryPath is %T",RegistryPath);

	// Export other driver entry points...
	DriverObject->DriverExtension->AddDevice = PassThruAddDevice;
	DriverObject->DriverUnload = PassThruUnload;

	for( int i=0; i<=IRP_MJ_MAXIMUM_FUNCTION; i++)
		DriverObject->MajorFunction[i] = PassThru;

	DriverObject->MajorFunction[IRP_MJ_PNP] = PassThruPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = PassThruPower;

	DebugPrintMsg("DriverEntry completed");

	return status;
}
#pragma code_seg() // end INIT section

//////////////////////////////////////////////////////////////////////////////

NTSTATUS PassThru(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp)
{
	DebugPrint("Pass through %I", Irp);

	// Just pass to lower driver
	IoSkipCurrentIrpStackLocation(Irp);
	PPASSTHRU_DEVICE_EXTENSION dx = (PPASSTHRU_DEVICE_EXTENSION)fdo->DeviceExtension;
	return IoCallDriver( dx->NextStackDevice, Irp);
}

//////////////////////////////////////////////////////////////////////////////
//	PassThruUnload
//
//	Description:
//		Unload the driver by removing any remaining objects, etc.
//
//	Arguments:
//		Pointer to the Driver object
//
//	Return Value:
//		None

VOID PassThruUnload(IN PDRIVER_OBJECT DriverObject)
{
	DebugPrintMsg("PassThruUnload");
	DebugPrintClose();
}

//////////////////////////////////////////////////////////////////////////////
