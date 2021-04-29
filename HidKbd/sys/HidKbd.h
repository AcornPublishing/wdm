//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	HidKbd example
/////////////////////////////////////////////////////////////////////////////
//	HidKbd.h			Common header
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
#include "c:\98ddk\src\hid\inc\hidclass.h"
#include "c:\98ddk\src\hid\inc\hidusage.h"
#include "c:\98ddk\src\hid\inc\hidpi.h"
#include "wdmguid.h"
#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////
//	DebugPrint and Guid headers

#include "DebugPrint.h"

#include "..\..\GUIDs.h"

/////////////////////////////////////////////////////////////////////////////
//	Our device extension

typedef struct _HIDKBD_DEVICE_EXTENSION
{
	PDEVICE_OBJECT	HidDevice;

	UNICODE_STRING HidSymLinkName;
	PHIDP_PREPARSED_DATA HidPreparsedData;
	USHORT HidInputReportLen;
	USHORT HidOutputReportLen;
	USHORT HidMaxReportLen;

	// Fields used for preallocated Read IRP
	PIRP HidIrp;
	PVOID HidReport;
	PMDL HidReportMdl;

	LONG OpenHandleCount;	// Count of open handles

} HIDKBD_DEVICE_EXTENSION, *PHIDKBD_DEVICE_EXTENSION;

/////////////////////////////////////////////////////////////////////////////
// Forward declarations of global functions

VOID HidKbdUnload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS RegisterForPnpNotification( IN PDRIVER_OBJECT DriverObject);

void UnregisterForPnpNotification();

NTSTATUS ReadHidKbdInputReport( PHIDKBD_DEVICE_EXTENSION dx, PFILE_OBJECT FileObject,
							    PVOID Buffer, ULONG& BytesTxd);


NTSTATUS HidKbdCreate(IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS HidKbdClose(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);
 
NTSTATUS HidKbdWrite(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS HidKbdRead(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS HidKbdDeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

#define FreeIfAllocated(x) if( (x)!=NULL) { ExFreePool(x); (x) = NULL; }

/////////////////////////////////////////////////////////////////////////////

NTSTATUS StartDevice(	IN PHIDKBD_DEVICE_EXTENSION dx,
						IN PCM_RESOURCE_LIST AllocatedResourcesTranslated);

VOID StopDevice(	IN PHIDKBD_DEVICE_EXTENSION dx);

/////////////////////////////////////////////////////////////////////////////

NTSTATUS CompleteIrp( IN PIRP Irp, IN NTSTATUS status, IN ULONG info);

/////////////////////////////////////////////////////////////////////////////
