//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998,1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm3 example
/////////////////////////////////////////////////////////////////////////////
//	init.cpp:		Driver initialization code
/////////////////////////////////////////////////////////////////////////////
//	DriverEntry		Initialisation entry point
//	Wdm3Unload		Unload driver routine
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	19-May-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "wdm3.h"

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
	DebugPrintInit("Wdm3 checked");
#else
	DebugPrintInit("Wdm3 free");
#endif

	DebugPrint("RegistryPath is %T",RegistryPath);

	InitializeEventLog(DriverObject);

	// Save a copy of our RegistryPath for WMI
	Wdm3RegistryPath.MaximumLength = RegistryPath->MaximumLength;
	Wdm3RegistryPath.Length = 0;
	Wdm3RegistryPath.Buffer = (PWSTR)ExAllocatePool( PagedPool, Wdm3RegistryPath.MaximumLength);
	if( Wdm3RegistryPath.Buffer==NULL) return STATUS_INSUFFICIENT_RESOURCES;

	RtlCopyUnicodeString( &Wdm3RegistryPath, RegistryPath);
	DebugPrint("Wdm3RegistryPath is %T",&Wdm3RegistryPath);

	// Export other driver entry points...
 	DriverObject->DriverExtension->AddDevice = Wdm3AddDevice;
 	DriverObject->DriverUnload = Wdm3Unload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = Wdm3Create;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = Wdm3Close;

	DriverObject->MajorFunction[IRP_MJ_PNP] = Wdm3Pnp;
	DriverObject->MajorFunction[IRP_MJ_POWER] = Wdm3Power;

	DriverObject->MajorFunction[IRP_MJ_READ] = Wdm3Read;
	DriverObject->MajorFunction[IRP_MJ_WRITE] = Wdm3Write;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = Wdm3DeviceControl;

	DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = Wdm3SystemControl;

	//	Initialise spin lock which protects access to shared memory buffer
	KeInitializeSpinLock(&BufferLock);

	DebugPrintMsg("DriverEntry completed");

	return status;
}
#pragma code_seg() // end INIT section

//////////////////////////////////////////////////////////////////////////////
//	Wdm3Unload
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

VOID Wdm3Unload(IN PDRIVER_OBJECT DriverObject)
{
	Wdm3EventMessage("Unload");

	// Free buffer (do not need to acquire spin lock)
	if( Buffer!=NULL)
		ExFreePool(Buffer);

	// Free registry path buffer
	if( Wdm3RegistryPath.Buffer!=NULL)
		ExFreePool(Wdm3RegistryPath.Buffer);

	DebugPrintMsg("Wdm3Unload");
	DebugPrintClose();
}

//////////////////////////////////////////////////////////////////////////////
#pragma code_seg() // end PAGE section
