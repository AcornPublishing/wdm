//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	WdmIo example
/////////////////////////////////////////////////////////////////////////////
//	init.cpp:		Driver initialization code
/////////////////////////////////////////////////////////////////////////////
//	DriverEntry		Initialisation entry point
//	WdmIoUnload		Unload driver routine
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "wdmIo.h"

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
	DebugPrintInit("WdmIo checked");
#else
	DebugPrintInit("WdmIo free");
#endif

	DebugPrint("RegistryPath is %T",RegistryPath);

	// Export other driver entry points...
 	DriverObject->DriverExtension->AddDevice = WdmIoAddDevice;
 	DriverObject->DriverStartIo = WdmIoStartIo;
 	DriverObject->DriverUnload = WdmIoUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = WdmIoCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = WdmIoClose;

	DriverObject->MajorFunction[IRP_MJ_PNP] = WdmIoPnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = WdmIoPower;

	DriverObject->MajorFunction[IRP_MJ_READ] = WdmIoRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = WdmIoWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = WdmIoDeviceControl;

	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = WdmIoSystemControl;

	DriverObject->MajorFunction[IRP_MJ_CLEANUP] = WdmIoDispatchCleanup;

	DebugPrintMsg("DriverEntry completed");

	return status;
}
#pragma code_seg() // end INIT section

//////////////////////////////////////////////////////////////////////////////
//	WdmIoUnload
//
//	Description:
//		Unload the driver by removing any remaining objects, etc.
//
//	Arguments:
//		Pointer to the Driver object
//
//	Return Value:
//		None

#pragma code_seg("PAGE") // start PAGE section

VOID WdmIoUnload(IN PDRIVER_OBJECT DriverObject)
{
	DebugPrintMsg("WdmIoUnload");
	DebugPrintClose();
}

//////////////////////////////////////////////////////////////////////////////
#pragma code_seg() // end PAGE section
