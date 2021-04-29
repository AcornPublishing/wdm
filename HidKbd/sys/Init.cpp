//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	HidKbd example
/////////////////////////////////////////////////////////////////////////////
//	init.cpp:			Driver initialization code
/////////////////////////////////////////////////////////////////////////////
//	DriverEntry			Initialisation entry point
//	HidKbdUnload		Unload driver routine
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "HidKbd.h"

/////////////////////////////////////////////////////////////////////////////

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
	DebugPrintInit("HidKbd checked");
#else
	DebugPrintInit("HidKbd free");
#endif

	DebugPrint("RegistryPath is %T",RegistryPath);

	// Export other driver entry points...
 	DriverObject->DriverUnload = HidKbdUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = HidKbdCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = HidKbdClose;

	DriverObject->MajorFunction[IRP_MJ_READ] = HidKbdRead;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = HidKbdWrite;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = HidKbdDeviceControl;

	RegisterForPnpNotification(DriverObject);

	DebugPrintMsg("DriverEntry completed");

	return status;
}
#pragma code_seg() // end INIT section

//////////////////////////////////////////////////////////////////////////////
//	HidKbdUnload
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

VOID HidKbdUnload(IN PDRIVER_OBJECT DriverObject)
{
	UnregisterForPnpNotification();

	DebugPrintMsg("HidKbdUnload");
	DebugPrintClose();
}

//////////////////////////////////////////////////////////////////////////////
#pragma code_seg() // end PAGE section
