//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm2 example
/////////////////////////////////////////////////////////////////////////////
//	init.cpp:		Driver initialization code
/////////////////////////////////////////////////////////////////////////////
//	DriverEntry		Initialisation entry point
//	Wdm2Unload		Unload driver routine
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "wdm2.h"

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
	DebugPrintInit("Wdm2 checked");
#else
	DebugPrintInit("Wdm2 free");
#endif

	DebugPrint("RegistryPath is %T",RegistryPath);

	// Export other driver entry points...
 	DriverObject->DriverExtension->AddDevice = Wdm2AddDevice;
 	DriverObject->DriverUnload = Wdm2Unload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = Wdm2Create;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Wdm2Close;

	DriverObject->MajorFunction[IRP_MJ_PNP] = Wdm2Pnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = Wdm2Power;

	DriverObject->MajorFunction[IRP_MJ_READ] = Wdm2Read;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Wdm2Write;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Wdm2DeviceControl;

	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = Wdm2SystemControl;

	//	Initialise spin lock which protects access to shared memory buffer
	KeInitializeSpinLock(&BufferLock);

	DebugPrintMsg("DriverEntry completed");

	return status;
}
#pragma code_seg() // end INIT section

//////////////////////////////////////////////////////////////////////////////
//	Wdm2Unload
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

VOID Wdm2Unload(IN PDRIVER_OBJECT DriverObject)
{
	// Free buffer (do not need to acquire spin lock)
	if( Buffer!=NULL)
		ExFreePool(Buffer);

	DebugPrintMsg("Wdm2Unload");
	DebugPrintClose();
}

//////////////////////////////////////////////////////////////////////////////
#pragma code_seg() // end PAGE section
