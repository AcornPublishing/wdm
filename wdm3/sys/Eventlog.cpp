//////////////////////////////////////////////////////////////////////////////
//	Copyright © 1998,1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	Wdm3 example
/////////////////////////////////////////////////////////////////////////////
//	eventlog.c:		Routines that give kernel-mode drivers
//					access to the system Event Log.
/////////////////////////////////////////////////////////////////////////////
//	Wdm3EventMessage	Report a WDM3_MESSAGE event
//	InitializeEventLog	Save DriverObject for future events
//	ReportEvent
//	GetWideStringSize
//	GetAnsiStringSize
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	28-Apr-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "Wdm3.h"

/////////////////////////////////////////////////////////////////////////////
// Data global to this module

PDRIVER_OBJECT SavedDriverObject = NULL;

/////////////////////////////////////////////////////////////////////////////

int GetWideStringSize( IN const wchar_t* String);
int GetAnsiStringSize( IN const char* str);

/////////////////////////////////////////////////////////////////////////////

void Wdm3EventMessage( const char* Msg)
{
	int MsgLen = GetAnsiStringSize(Msg);
	int wMsgLen = MsgLen*2;
	PWSTR wMsg = (PWSTR)ExAllocatePool(NonPagedPool,wMsgLen);
	if( wMsg==NULL) return;

	// Brutally make into a wide string
	for( int i=0;i<MsgLen;i++)
		wMsg[i] = (WCHAR)(unsigned char)Msg[i];

	PWSTR Strings[1] = { wMsg };
	LogEvent( WDM3_MESSAGE, NULL,	// IRP
				NULL, 0,			// dump data
				Strings, 1);		// strings

	ExFreePool(wMsg);
}

/////////////////////////////////////////////////////////////////////////////
//	InitializeEventLog:	Save DriverObject for future events

void InitializeEventLog( IN PDRIVER_OBJECT DriverObject)
{
	SavedDriverObject = DriverObject;

	// Log a message saying that logging is started.
	LogEvent( WDM3_MSG_LOGGING_STARTED, NULL,	// IRP
				NULL, 0,						// dump data
				NULL, 0);						// strings
}

/////////////////////////////////////////////////////////////////////////////
//	ReportEvent:	Write event.
//
// IRQL:
//		<= DISPATCH_LEVEL
//
// Arguments:
//		Error message code
//		Address of IRP (or NULL)
//		Address of dump data array (or NULL)
//		Count of ULONGs in dump data array (or zero)
//		Address of array of insertion strings
//		Count of insertion strings (or zero)

bool LogEvent(	IN NTSTATUS	ErrorCode,
				IN PIRP		Irp,
				IN ULONG	DumpData[],
				IN int		DumpDataCount,
				IN PWSTR	Strings[],
				IN int		StringCount)
{
	if( SavedDriverObject==NULL) return false;

	// Start working out size of complete event packet
	int size = sizeof(IO_ERROR_LOG_PACKET);

	// Add in dump data size.
	// Less one as DumpData already has 1 ULONG in IO_ERROR_LOG_PACKET
	if( DumpDataCount>0)
		size += sizeof(ULONG) * (DumpDataCount-1); 

	// Add in space needed for insertion strings (inc terminating NULLs)
	int* StringSizes = NULL;
	if( StringCount>0)
	{
		StringSizes = (int*)ExAllocatePool(NonPagedPool,StringCount*sizeof(int));
		if( StringSizes==NULL) return false;

		// Remember each string size
		for( int i=0; i<StringCount; i++)
		{
			StringSizes[i] = (int)GetWideStringSize(Strings[i]);
			size += StringSizes[i];
		}
	}

	if( size>ERROR_LOG_MAXIMUM_SIZE)	// 0x98!
	{
		if( StringSizes!=NULL) ExFreePool(StringSizes);
		return false;
	}

	// Try to allocate the packet
	PIO_ERROR_LOG_PACKET Packet = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry( SavedDriverObject, size);
	if( Packet==NULL)
	{
		if( StringSizes!=NULL) ExFreePool(StringSizes);
		return false;
	}

	// Fill in standard parts of the packet
	Packet->ErrorCode = ErrorCode;
	Packet->UniqueErrorValue = 0;

	// Fill in IRP related fields
	Packet->MajorFunctionCode = 0;
	Packet->RetryCount = 0;
	Packet->FinalStatus = 0;
	Packet->SequenceNumber = 0;
	Packet->IoControlCode = 0;
	if( Irp!=NULL)
	{
		PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
		
		Packet->MajorFunctionCode = IrpStack->MajorFunction;
		Packet->FinalStatus = Irp->IoStatus.Status;

		if( IrpStack->MajorFunction==IRP_MJ_DEVICE_CONTROL ||
			IrpStack->MajorFunction==IRP_MJ_INTERNAL_DEVICE_CONTROL)
			Packet->IoControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
	}

	// Fill in dump data
	if( DumpDataCount>0)
	{
		Packet->DumpDataSize = (USHORT)(sizeof(ULONG)*DumpDataCount);

		for( int i=0; i<DumpDataCount; i++)
			Packet->DumpData[i] = DumpData[i];
	}
	else Packet->DumpDataSize = 0;

	// Fill in insertion strings after DumpData
	Packet->NumberOfStrings = (USHORT)StringCount;

	if( StringCount>0)
	{
		Packet->StringOffset = sizeof(IO_ERROR_LOG_PACKET) +
								(DumpDataCount-1) * sizeof(ULONG);

		PUCHAR pInsertionString = ((PUCHAR)Packet) + Packet->StringOffset;

		// Add each new string to the end
		for( int i=0; i<StringCount; i++)
		{
			RtlCopyMemory( pInsertionString, Strings[i], StringSizes[i]);
			pInsertionString += StringSizes[i];
		}
	}

	// Log the message
	IoWriteErrorLogEntry(Packet);

	if( StringSizes!=NULL) ExFreePool(StringSizes);

	return true;
}

/////////////////////////////////////////////////////////////////////////////
//	GetWideStringSize:	Get size of string in bytes inc terminating NULL

int GetWideStringSize( IN const wchar_t* str)
{
	int len = 0;
	while( *str++!=L'\0')
		len++;
	return (len+1)*sizeof(WCHAR);
}

/////////////////////////////////////////////////////////////////////////////
//	GetAnsiStringSize:	Get size of string in bytes inc terminating NULL

int GetAnsiStringSize( IN const char* str)
{
	int len = 0;
	while(*str++!='\0')
		len++;
	return len+1;
}

/////////////////////////////////////////////////////////////////////////////

