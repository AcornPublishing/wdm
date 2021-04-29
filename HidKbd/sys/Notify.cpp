//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	HidKbd example
/////////////////////////////////////////////////////////////////////////////
//	notify.cpp:		PnP Notification handling routines
/////////////////////////////////////////////////////////////////////////////
//	ShowButtonCaps					Print button capabilities
//	CallHidIoctl					Make an IOCTL call to the HID class driver
//	GetPreparsedData				Get the HID device preparsed data
//	GetCapabilities					Get capabilities to see if a HID keyboard
//	CreateDevice					If found device is a HID keyboard, create our one device
//	DeleteDevice					Delete our device, if appropriate
//	HidKbdDicCallback				PnP Notify callback for device interface changes
//	RegisterForPnpNotification		Ask for device interface change events
//	UnregisterForPnpNotification	Stop device interface change events
//	ReadHidKbdInputReport			Read a HID keyboard input report
/////////////////////////////////////////////////////////////////////////////
// Routines which use a preallocated IRP (commented out)
//	ReadComplete			Completion routine for our read IRP
//	ReadHidKbdInputReport	Read a HID keyboard input report
//	SetupHidIrp				Allocate IRP, buffer and MDL
//	RemoveHidIrp			Deallocate IRP, buffer and MDL
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#define INITGUID // initialize GUIDs in this module

#include "HidKbd.h"

///////////////////////////////////////////////////////////////////////////////////

PDEVICE_OBJECT HidKbdDo = NULL;
PVOID diNotificationEntry = NULL;

///////////////////////////////////////////////////////////////////////////////////
//	ShowButtonCaps:	Print button capabilities

void ShowButtonCaps( char* Msg, HIDP_REPORT_TYPE ReportType, USHORT NumCaps, PHIDP_PREPARSED_DATA HidPreparsedData)
{
	if( NumCaps==0) return;

	DebugPrint("%s", Msg);

	HIDP_BUTTON_CAPS* ButtonCaps = (HIDP_BUTTON_CAPS*)ExAllocatePool(PagedPool,NumCaps*sizeof(HIDP_BUTTON_CAPS));
	if( ButtonCaps==NULL) return;

	NTSTATUS status = HidP_GetButtonCaps( ReportType, ButtonCaps, &NumCaps, HidPreparsedData);
	if( status==HIDP_STATUS_SUCCESS)
	{
		for( USHORT i=0; i<NumCaps; i++)
		{
			DebugPrint("ButtonCaps[%d].UsagePage %d", i, ButtonCaps[i].UsagePage);
			if( ButtonCaps[i].IsRange)
				DebugPrint("             .Usages    %d..%d", ButtonCaps[i].Range.UsageMin, ButtonCaps[i].Range.UsageMax);
			else
				DebugPrint("             .Usage     %d", ButtonCaps[i].NotRange.Usage);
		}
	}
	ExFreePool(ButtonCaps);
}

///////////////////////////////////////////////////////////////////////////////////
//	CallHidIoctl:	Make an IOCTL call to the HID class driver

NTSTATUS CallHidIoctl( IN PDEVICE_OBJECT HidDevice, IN ULONG IoControlCode,
				  OUT PVOID Output, IN ULONG OutputLen)
{
	IO_STATUS_BLOCK IoStatus;
	KEVENT event;

	// Initialise IRP completion event
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	// Build Internal IOCTL IRP
	PIRP Irp = IoBuildDeviceIoControlRequest(
					IoControlCode, HidDevice,
					NULL, 0,	// Input buffer
					Output, OutputLen,	// Output buffer
					TRUE, &event, &IoStatus);
	// Call the driver and wait for completion if necessary
	NTSTATUS status = IoCallDriver( HidDevice, Irp);
	if (status == STATUS_PENDING)
	{
		DebugPrintMsg("CallHidIoctl: waiting for completion");
		status = KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL);
	}
	else
		IoStatus.Status = status;

	// return IRP completion status
	status = IoStatus.Status;
	DebugPrint("CallHidIoctl: status %x", status);
	return status;
}

///////////////////////////////////////////////////////////////////////////////////
//	GetPreparsedData:	Get the HID device preparsed data

bool GetPreparsedData( IN PDEVICE_OBJECT HidDevice, OUT PHIDP_PREPARSED_DATA HidPreparsedData)
{
	HID_COLLECTION_INFORMATION HidCi;
	NTSTATUS status = CallHidIoctl( HidDevice, IOCTL_HID_GET_COLLECTION_INFORMATION,
								&HidCi, sizeof(HidCi));
	if( !NT_SUCCESS(status))
	{
		DebugPrint("IOCTL_HID_GET_COLLECTION_INFORMATION failed %x", status);
		return false;
	}
	DebugPrint("HID attributes: VendorID=%4x, ProductID=%4x, VersionNumber=%4x",
				HidCi.VendorID, HidCi.ProductID, HidCi.VersionNumber);

	ULONG PreparsedDatalen = HidCi.DescriptorSize;
	DebugPrint("PreparsedDatalen %d",PreparsedDatalen);
	HidPreparsedData = (PHIDP_PREPARSED_DATA)ExAllocatePool( NonPagedPool, PreparsedDatalen);
	if( HidPreparsedData==NULL)
	{
		DebugPrintMsg("No memory");
		return false;
	}

	status = CallHidIoctl( HidDevice, IOCTL_HID_GET_COLLECTION_DESCRIPTOR,
						HidPreparsedData, PreparsedDatalen);
	if( !NT_SUCCESS(status))
	{
		DebugPrint("IOCTL_HID_GET_COLLECTION_DESCRIPTOR failed %x", status);
		return false;
	}
	
	return true;
}

///////////////////////////////////////////////////////////////////////////////////
//	GetCapabilities:	Get capabilities to see if a HID keyboard
//	Return true if HID keyboard found

bool GetCapabilities( IN PDEVICE_OBJECT HidDevice, PHIDP_PREPARSED_DATA& HidPreparsedData,
					  USHORT& InputReportLen, USHORT& OutputReportLen)
{
	HidPreparsedData = NULL;
	InputReportLen = 0;
	OutputReportLen = 0;
	if( !GetPreparsedData( HidDevice, HidPreparsedData))
	{
		DebugPrintMsg("GetPreparsedData failed");
		return false;
	}
	HIDP_CAPS HidCaps;
	bool found = false;
	NTSTATUS status = HidP_GetCaps( HidPreparsedData, &HidCaps);
	if( status==HIDP_STATUS_SUCCESS)
	{
		if( HidCaps.UsagePage==HID_USAGE_PAGE_GENERIC &&
			HidCaps.Usage==HID_USAGE_GENERIC_KEYBOARD)
		{
			DebugPrintMsg("Found HID keyboard");
			found = true;
		}
		InputReportLen = HidCaps.InputReportByteLength;
		OutputReportLen = HidCaps.OutputReportByteLength;
		DebugPrint("InputReportByteLength %d", HidCaps.InputReportByteLength);
		DebugPrint("OutputReportByteLength %d", HidCaps.OutputReportByteLength);
		DebugPrint("FeatureReportByteLength %d", HidCaps.FeatureReportByteLength);

		DebugPrint("NumberLinkCollectionNodes %d", HidCaps.NumberLinkCollectionNodes);

		DebugPrint("NumberInputButtonCaps %d", HidCaps.NumberInputButtonCaps);
		DebugPrint("NumberInputValueCaps %d", HidCaps.NumberInputValueCaps);
		DebugPrint("NumberOutputButtonCaps %d", HidCaps.NumberOutputButtonCaps);
		DebugPrint("NumberOutputValueCaps %d", HidCaps.NumberOutputValueCaps);
		DebugPrint("NumberFeatureButtonCaps %d", HidCaps.NumberFeatureButtonCaps);
		DebugPrint("NumberFeatureValueCaps %d", HidCaps.NumberFeatureValueCaps);
		
		ShowButtonCaps( "Input button capabilities", HidP_Input, HidCaps.NumberInputButtonCaps, HidPreparsedData);
		ShowButtonCaps( "Output button capabilities", HidP_Output, HidCaps.NumberOutputButtonCaps, HidPreparsedData);
	}

	// Out of interest, get ring buffer size
	{
		HID_DRIVER_CONFIG HidConfig;
		HidConfig.Size = sizeof(HidConfig);
		NTSTATUS status = CallHidIoctl( HidDevice, IOCTL_HID_GET_DRIVER_CONFIG,
									&HidConfig, sizeof(HidConfig));
		if( NT_SUCCESS(status))
			DebugPrint("RingBufferSize %d",HidConfig.RingBufferSize);
		else
			DebugPrintMsg("IOCTL_HID_GET_DRIVER_CONFIG failed");
	}

	return found;
}

/////////////////////////////////////////////////////////////////////////////
//	CreateDevice:	If found device is a HID keyboard, create our one device
//
//	The DDK says that PnP Notification callback code must not block.
//	The call to GetCapabilities might block so all this code ought to be removed
//	to a system worker thread.

void CreateDevice( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING HidSymLinkName)
{
	if( HidKbdDo!=NULL)
	{
		DebugPrintMsg("Already got HidKbdDo");
		return;
	}
	PFILE_OBJECT HidFileObject = NULL;
	PDEVICE_OBJECT HidDevice;
	NTSTATUS status = IoGetDeviceObjectPointer( HidSymLinkName, FILE_ALL_ACCESS,
		&HidFileObject, &HidDevice);
	if( !NT_SUCCESS(status))
	{
		DebugPrint("IoGetDeviceObjectPointer failed %x", status);
		return;
	}

	// Close file object
	ObDereferenceObject(HidFileObject);

	// Inspect HID capabilities here
	PHIDP_PREPARSED_DATA HidPreparsedData = NULL;
	USHORT HidInputReportLen, HidOutputReportLen;
	if( !GetCapabilities( HidDevice, HidPreparsedData, HidInputReportLen, HidOutputReportLen))
	{
		DebugPrintMsg("GetCapabilities failed");
		FreeIfAllocated(HidPreparsedData);
		return;
	}

	// Allocate a buffer for the device ext HidSymLinkName
	PWSTR HidSymLinkNameBuffer = (PWSTR)ExAllocatePool( NonPagedPool,HidSymLinkName->MaximumLength);
	if( HidSymLinkNameBuffer==NULL)
	{
		FreeIfAllocated(HidPreparsedData);
		return;
	}

	// Reference device object
	status = ObReferenceObjectByPointer( HidDevice, FILE_ALL_ACCESS, NULL, KernelMode);
	if( !NT_SUCCESS(status))
	{
		DebugPrintMsg("ObReferenceObjectByPointer failed");
		FreeIfAllocated(HidSymLinkNameBuffer);
		FreeIfAllocated(HidPreparsedData);
		return;
	}

#define	NT_DEVICE_NAME	L"\\Device\\HidKbd"
#define	SYM_LINK_NAME	L"\\DosDevices\\HidKbd"

	// Initialise NT and Symbolic link names
	UNICODE_STRING deviceName, linkName;
	RtlInitUnicodeString( &deviceName, NT_DEVICE_NAME);
	RtlInitUnicodeString( &linkName, SYM_LINK_NAME);

	// Create our device object
	status = IoCreateDevice( DriverObject, sizeof(HIDKBD_DEVICE_EXTENSION), &deviceName,
								FILE_DEVICE_KEYBOARD, 0, FALSE, &HidKbdDo);
	if( !NT_SUCCESS(status))
	{
		HidKbdDo = NULL;
		FreeIfAllocated(HidSymLinkNameBuffer);
		FreeIfAllocated(HidPreparsedData);
		ObDereferenceObject(HidDevice);
		return;
	}

	// Set up our device extension
	PHIDKBD_DEVICE_EXTENSION dx = (PHIDKBD_DEVICE_EXTENSION)HidKbdDo->DeviceExtension;
	dx->HidDevice = HidDevice;
	dx->HidPreparsedData = HidPreparsedData;
	dx->HidInputReportLen = HidInputReportLen;
	dx->HidOutputReportLen = HidOutputReportLen;
	dx->HidMaxReportLen = 0;
	dx->HidIrp = NULL;
	dx->HidReport = NULL;
	dx->HidReportMdl = NULL;

	dx->HidSymLinkName.Length = 0;
	dx->HidSymLinkName.MaximumLength = HidSymLinkName->MaximumLength;
	dx->HidSymLinkName.Buffer = HidSymLinkNameBuffer;
	RtlCopyUnicodeString( &dx->HidSymLinkName, HidSymLinkName);

	// Create a symbolic link so our device is visible to Win32...
	DebugPrint("Creating symbolic link %T",&linkName);
	status = IoCreateSymbolicLink( &linkName, &deviceName);
	if( !NT_SUCCESS(status)) 
	{
		DebugPrintMsg("Could not create symbolic link");
		FreeIfAllocated(dx->HidSymLinkName.Buffer);
		FreeIfAllocated(HidPreparsedData);
		IoDeleteDevice(HidKbdDo);
		ObDereferenceObject(HidDevice);
		HidKbdDo = NULL;
		return;
	}

	HidKbdDo->Flags &= ~DO_DEVICE_INITIALIZING;
	HidKbdDo->Flags |= DO_BUFFERED_IO;

	HidKbdDo->StackSize = HidDevice->StackSize+1;

//	SetupHidIrp( dx, HidKbdDo->StackSize);

	DebugPrintMsg("Device created OK");
}

/////////////////////////////////////////////////////////////////////////////
//	DeleteDevice:	Delete our device, if appropriate
//					If device name passed then check it matches ours
//					If no device name passed then always delete device

void DeleteDevice( IN PUNICODE_STRING HidSymLinkName)
{
	if( HidKbdDo==NULL)
		return;

	PHIDKBD_DEVICE_EXTENSION dx = (PHIDKBD_DEVICE_EXTENSION)HidKbdDo->DeviceExtension;
	if( HidSymLinkName!=NULL &&
		RtlCompareUnicodeString( HidSymLinkName, &dx->HidSymLinkName, FALSE)!=0)
	{
		DebugPrintMsg("DeleteDevice: symbolic link does not match our device");
		return;
	}

	DebugPrintMsg("Deleting our device");

//	RemoveHidIrp(dx);

	FreeIfAllocated(dx->HidPreparsedData);
	FreeIfAllocated(dx->HidSymLinkName.Buffer);

	// Initialise Symbolic link names
	UNICODE_STRING linkName;
	RtlInitUnicodeString( &linkName, SYM_LINK_NAME);

	// Remove symbolic link
	DebugPrint("Deleting symbolic link %T",&linkName);
	IoDeleteSymbolicLink( &linkName);
	
	ObDereferenceObject(dx->HidDevice);
	IoDeleteDevice(HidKbdDo);

	HidKbdDo = NULL;
}

/////////////////////////////////////////////////////////////////////////////
//	HidKbdDicCallback:	PnP Notify callback for device interface changes
//						Process device arrival and removal messages

NTSTATUS HidKbdDicCallback(
	IN PVOID NotificationStructure,
	IN PVOID Context)
{
	PDEVICE_INTERFACE_CHANGE_NOTIFICATION dicn = (PDEVICE_INTERFACE_CHANGE_NOTIFICATION)NotificationStructure;
	PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Context;

	if( IsEqualGUID( dicn->Event, GUID_DEVICE_INTERFACE_ARRIVAL))
	{
		DebugPrint("Device arrival: %T", dicn->SymbolicLinkName);
		CreateDevice( DriverObject, dicn->SymbolicLinkName);
	}
	else if( IsEqualGUID( dicn->Event, GUID_DEVICE_INTERFACE_REMOVAL))
	{
		DebugPrint("Device removal: %T", dicn->SymbolicLinkName);
		DeleteDevice( dicn->SymbolicLinkName);
	}
	else
		DebugPrint("Some other device event: %T", dicn->SymbolicLinkName);

	return STATUS_SUCCESS;
}

#pragma code_seg("INIT") // start INIT section

/////////////////////////////////////////////////////////////////////////////
//	RegisterForPnpNotification:	Ask for device interface change events

NTSTATUS RegisterForPnpNotification( IN PDRIVER_OBJECT DriverObject)
{
	DebugPrintMsg("RegisterForPnpNotification");

	NTSTATUS status = IoRegisterPlugPlayNotification( EventCategoryDeviceInterfaceChange,
		PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
		(PVOID)&GUID_CLASS_INPUT, DriverObject,
		HidKbdDicCallback, DriverObject,
		&diNotificationEntry);
	return status;
}
#pragma code_seg() // end INIT section

//////////////////////////////////////////////////////////////////////////////
//	UnregisterForPnpNotification:	Stop device interface change events

void UnregisterForPnpNotification()
{
	DeleteDevice(NULL);
	DebugPrintMsg("UnregisterForPnpNotification");
	if( diNotificationEntry!=NULL)
	{
		IoUnregisterPlugPlayNotification(diNotificationEntry);
		diNotificationEntry = NULL;
	}
}

//////////////////////////////////////////////////////////////////////////////
//	ReadHidKbdInputReport:	Read a HID keyboard input report
//
//	Version which allocates an IRP each time

NTSTATUS ReadHidKbdInputReport( PHIDKBD_DEVICE_EXTENSION dx, PFILE_OBJECT FileObject,
							    PVOID Buffer, ULONG& BytesTxd)
{
	BytesTxd = 0;
	if( HidKbdDo==NULL || dx->HidInputReportLen==0) return STATUS_NO_MEDIA_IN_DEVICE;

	IO_STATUS_BLOCK IoStatus;
	IoStatus.Information = 0;
	KEVENT event;
	LARGE_INTEGER FilePointer;
	FilePointer.QuadPart = 0i64;

	// Initialise IRP completion event
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	PIRP Irp = IoBuildSynchronousFsdRequest(
		IRP_MJ_READ, dx->HidDevice,
		Buffer, dx->HidInputReportLen, &FilePointer,
		&event, &IoStatus);
	if( Irp==NULL) return STATUS_INSUFFICIENT_RESOURCES;

	// Store file object pointer
	PIO_STACK_LOCATION IrpStack = IoGetNextIrpStackLocation(Irp);
	IrpStack->FileObject = FileObject;

	// Call the driver and wait for completion if necessary
	NTSTATUS status = IoCallDriver( dx->HidDevice, Irp);
	if (status == STATUS_PENDING)
	{
		status = KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL);
		status = IoStatus.Status;
	}

	// return IRP completion status
	DebugPrint("ReadHidKbdInputReport: status %x", status);
	BytesTxd = IoStatus.Information;
	return status;
}

//////////////////////////////////////////////////////////////////////////////
// The following code shows how to use a preallocated IRP for reads
//////////////////////////////////////////////////////////////////////////////
/*

void SetupHidIrp( IN PHIDKBD_DEVICE_EXTENSION dx, IN CCHAR StackSize);
void RemoveHidIrp( IN PHIDKBD_DEVICE_EXTENSION dx);

//////////////////////////////////////////////////////////////////////////////
//	ReadComplete:	Completion routine for our read IRP

NTSTATUS ReadComplete( IN PDEVICE_OBJECT fdo, IN PIRP Irp, IN PKEVENT Event)
{
	KeSetEvent(Event, 0, FALSE);
	return STATUS_MORE_PROCESSING_REQUIRED;
}

//////////////////////////////////////////////////////////////////////////////
//	ReadHidKbdInputReport:	Read a HID keyboard input report
//
//	Version that uses preallocated IRP

NTSTATUS ReadHidKbdInputReport( PHIDKBD_DEVICE_EXTENSION dx, PFILE_OBJECT FileObject,
							    PVOID Buffer, ULONG& BytesTxd)
{
	BytesTxd = 0;
	if( HidKbdDo==NULL || dx->HidIrp==NULL || dx->HidReport==NULL)
	{
		DebugPrintMsg("No HidIrp");
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlZeroMemory( dx->HidReport, dx->HidMaxReportLen);

	// Initialise IRP completion event
	KEVENT event;
	KeInitializeEvent(&event, NotificationEvent, FALSE);

	// Initialise IRP
	UCHAR AllocationFlags = dx->HidIrp->AllocationFlags;
	IoInitializeIrp( dx->HidIrp, IoSizeOfIrp(HidKbdDo->StackSize), HidKbdDo->StackSize);
	dx->HidIrp->AllocationFlags = AllocationFlags;

	dx->HidIrp->MdlAddress = dx->HidReportMdl;
//	dx->HidIrp->AssociatedIrp.SystemBuffer = dx->HidReport;

	PIO_STACK_LOCATION IrpStack = IoGetNextIrpStackLocation(dx->HidIrp);
	IrpStack->MajorFunction = IRP_MJ_READ;
	IrpStack->Parameters.Read.Key = 0;
	IrpStack->Parameters.Read.Length = dx->HidInputReportLen;
	IrpStack->Parameters.Read.ByteOffset.QuadPart = 0;
	IrpStack->FileObject = FileObject;

	IoSetCompletionRoutine( dx->HidIrp, (PIO_COMPLETION_ROUTINE)ReadComplete, &event, TRUE, TRUE, TRUE);

	NTSTATUS status = IoCallDriver( dx->HidDevice, dx->HidIrp);
	if (status == STATUS_PENDING)
	{
		KeWaitForSingleObject( &event, Suspended, KernelMode, FALSE, NULL);
		status = dx->HidIrp->IoStatus.Status;
	}

	// return IRP completion status
	DebugPrint("ReadHidKbdInputReport: status %x", status);
	BytesTxd = dx->HidIrp->IoStatus.Information;
	if( BytesTxd>0)
		RtlCopyMemory( Buffer, dx->HidReport, BytesTxd);
	return status;
}

/////////////////////////////////////////////////////////////////////////////
//	SetupHidIrp:	Allocate IRP, buffer and MDL

void SetupHidIrp( IN PHIDKBD_DEVICE_EXTENSION dx, IN CCHAR StackSize)
{
	dx->HidMaxReportLen = dx->HidInputReportLen;
	if( dx->HidOutputReportLen > dx->HidMaxReportLen)
		dx->HidMaxReportLen = dx->HidOutputReportLen;
	DebugPrint("Setting up HidIrp etc %d", dx->HidMaxReportLen);
	if( dx->HidMaxReportLen==0) return;

	dx->HidReport = ExAllocatePool( NonPagedPool, dx->HidMaxReportLen);
	if( dx->HidReport==NULL) return;

	dx->HidIrp = IoAllocateIrp( StackSize, FALSE);
	if( dx->HidIrp==NULL) return;

	dx->HidReportMdl = IoAllocateMdl( dx->HidReport, dx->HidMaxReportLen, FALSE, FALSE, NULL);
	if( dx->HidReportMdl==NULL)
	{
		IoFreeIrp(dx->HidIrp);
		dx->HidIrp = NULL;
	}

	DebugPrint("SetupHidIrp: HidReport %x HidIrp %x HidReportMdl %x",
		dx->HidReport, dx->HidIrp, dx->HidReportMdl);
}

/////////////////////////////////////////////////////////////////////////////
//	RemoveHidIrp:	Deallocate IRP, buffer and MDL

void RemoveHidIrp( IN PHIDKBD_DEVICE_EXTENSION dx)
{
	DebugPrintMsg("Removing HidIrp etc");
	if( dx->HidReportMdl!=NULL)
	{
		IoFreeMdl(dx->HidReportMdl);
		dx->HidReportMdl = NULL;
	}
	if( dx->HidIrp!=NULL)
	{
		IoFreeIrp(dx->HidIrp);
		dx->HidIrp = NULL;
	}
	if( dx->HidReport!=NULL)
	{
		ExFreePool(dx->HidReport);
		dx->HidReport = NULL;
	}
}
*/
/////////////////////////////////////////////////////////////////////////////
