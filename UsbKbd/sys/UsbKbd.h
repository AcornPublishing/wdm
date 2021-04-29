//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	UsbKbd example
/////////////////////////////////////////////////////////////////////////////
//	UsbKbd.h			Common header
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	27-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
//	Include WDM standard header with C linkage

#ifdef __cplusplus
extern "C"
{
#endif
#include "wdm.h"
#include "usb100.h"
#include "usbdi.h"
#include "usbdlib.h"
#ifdef __cplusplus
}
#endif


/////////////////////////////////////////////////////////////////////////////
//	DebugPrint and Guid headers

#include "DebugPrint.h"

#include "..\..\GUIDs.h"

/////////////////////////////////////////////////////////////////////////////
//	Our device extension

typedef struct _USBKBD_DEVICE_EXTENSION
{
	PDEVICE_OBJECT	fdo;
	PDEVICE_OBJECT	pdo;
	PDEVICE_OBJECT	NextStackDevice;
	UNICODE_STRING	ifSymLinkName;

	bool GotResources;	// Not stopped
	bool Paused;		// Stop or remove pending
	bool IODisabled;	// Paused or stopped
//	bool InterruptsEnabled;

	LONG OpenHandleCount;	// Count of open handles

	LONG UsageCount;		// Pending I/O Count
	bool Stopping;			// In process of stopping
	KEVENT StoppingEvent;	// Set when all pending I/O complete

	USBD_CONFIGURATION_HANDLE UsbConfigurationHandle;	// Selected Configuration handle
	USBD_PIPE_HANDLE UsbPipeHandle;	// Handle to input interrupt pipe
	ULONG UsbTimeout;	// Read timeout in seconds

	// Resource allocations
	bool GotPortOrMemory;
	bool PortInIOSpace;
	bool PortNeedsMapping;
	PUCHAR PortBase;
	PHYSICAL_ADDRESS PortStartAddress;
	ULONG PortLength;

	bool GotInterrupt;
	ULONG Vector;
	KIRQL Irql;
	KINTERRUPT_MODE Mode;
	KAFFINITY Affinity;
	PKINTERRUPT InterruptObject;

} USBKBD_DEVICE_EXTENSION, *PUSBKBD_DEVICE_EXTENSION;

/////////////////////////////////////////////////////////////////////////////
// Forward declarations of global functions

VOID UsbKbdUnload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS UsbKbdPower(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS UsbKbdPnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS UsbKbdAddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo);

NTSTATUS UsbKbdCreate(IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS UsbKbdClose(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);
 
NTSTATUS UsbKbdWrite(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS UsbKbdRead(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS UsbKbdDeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

NTSTATUS UsbKbdSystemControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

/////////////////////////////////////////////////////////////////////////////

bool LockDevice( IN PUSBKBD_DEVICE_EXTENSION dx);
void UnlockDevice( IN PUSBKBD_DEVICE_EXTENSION dx);

NTSTATUS StartDevice(	IN PUSBKBD_DEVICE_EXTENSION dx,
						IN PCM_RESOURCE_LIST AllocatedResourcesTranslated);

VOID StopDevice( IN PUSBKBD_DEVICE_EXTENSION dx);

NTSTATUS ForwardIrpAndWait( IN PDEVICE_OBJECT fdo, IN PIRP Irp);

/////////////////////////////////////////////////////////////////////////////
//	USB routines

NTSTATUS UsbResetDevice( IN PUSBKBD_DEVICE_EXTENSION dx);

NTSTATUS UsbGetUsbInfo( IN PUSBKBD_DEVICE_EXTENSION dx);

NTSTATUS UsbGetDeviceDescriptor( IN PUSBKBD_DEVICE_EXTENSION dx,
								 OUT PUSB_DEVICE_DESCRIPTOR& deviceDescriptor,
								 OUT ULONG& Size);

NTSTATUS UsbGetConfigurationDescriptors( IN PUSBKBD_DEVICE_EXTENSION dx, 
										 OUT PUSB_CONFIGURATION_DESCRIPTOR& descriptors,
										 IN UCHAR ConfigIndex,
										 OUT ULONG& DescriptorsSize);

NTSTATUS UsbGetSpecifiedDescriptor( IN PUSBKBD_DEVICE_EXTENSION dx,
									OUT PVOID& Descriptor,
									IN UCHAR DescriptorType,
									IN OUT ULONG& Size);

NTSTATUS UsbSelectConfiguration( IN PUSBKBD_DEVICE_EXTENSION dx);

NTSTATUS UsbDeselectConfiguration( IN PUSBKBD_DEVICE_EXTENSION dx);

NTSTATUS UsbDoInterruptTransfer( IN PUSBKBD_DEVICE_EXTENSION dx, IN PVOID Buffer, ULONG& BufferSize);

NTSTATUS UsbGetStatuses( IN PUSBKBD_DEVICE_EXTENSION dx,
						 OUT PUCHAR Statuses,
						 IN ULONG Size);

NTSTATUS UsbGetFrameInfo( IN PUSBKBD_DEVICE_EXTENSION dx,
						  OUT ULONG& FrameLength,
						  OUT ULONG& FrameNumber,
						  OUT ULONG& FrameAlterNumber);

NTSTATUS UsbSendOutputReport( IN PUSBKBD_DEVICE_EXTENSION dx, IN UCHAR OutputData);

NTSTATUS UsbGetIdleRate( IN PUSBKBD_DEVICE_EXTENSION dx);

#define FreeIfAllocated(x) if( (x)!=NULL) { ExFreePool(x); (x) = NULL; }

/////////////////////////////////////////////////////////////////////////////

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info);

/////////////////////////////////////////////////////////////////////////////
