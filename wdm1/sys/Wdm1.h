//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm1 example
/////////////////////////////////////////////////////////////////////////////
//	wdm1.h			Common header
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
#ifdef __cplusplus
}
#endif

/////////////////////////////////////////////////////////////////////////////
//	DebugPrint and Guid headers

#include "DebugPrint.h"

#include "GUIDs.h"

/////////////////////////////////////////////////////////////////////////////
//	Spin lock to protect access to shared memory buffer

extern KSPIN_LOCK BufferLock;
extern PUCHAR Buffer;

/////////////////////////////////////////////////////////////////////////////
//	Our device extension

typedef struct _WDM1_DEVICE_EXTENSION
{
	PDEVICE_OBJECT	fdo;
	PDEVICE_OBJECT	NextStackDevice;
	UNICODE_STRING	ifSymLinkName;

} WDM1_DEVICE_EXTENSION, *PWDM1_DEVICE_EXTENSION;

/////////////////////////////////////////////////////////////////////////////
// Forward declarations of global functions

VOID Wdm1Unload(IN PDRIVER_OBJECT DriverObject);

NTSTATUS Wdm1Power(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm1Pnp(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm1AddDevice(	IN PDRIVER_OBJECT DriverObject,
						IN PDEVICE_OBJECT pdo);

NTSTATUS Wdm1Create(IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm1Close(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);
 
NTSTATUS Wdm1Write(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm1Read(	IN PDEVICE_OBJECT fdo,
					IN PIRP Irp);

NTSTATUS Wdm1DeviceControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

NTSTATUS Wdm1SystemControl(	IN PDEVICE_OBJECT fdo,
							IN PIRP Irp);

/////////////////////////////////////////////////////////////////////////////

NTSTATUS CompleteIrp( PIRP Irp, NTSTATUS status, ULONG info);

/////////////////////////////////////////////////////////////////////////////
