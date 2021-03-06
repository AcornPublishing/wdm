//////////////////////////////////////////////////////////////////////////////
//	Copyright ? 1999 Chris Cant, PHD Computer Consultants Ltd
//	WDM Book for R&D Books, Miller Freeman Inc
//
//	WdmIo example
/////////////////////////////////////////////////////////////////////////////
//	DeviceIo.cpp:	Routines that interact with a device
/////////////////////////////////////////////////////////////////////////////
//	StartDevice			Start the device
//*	RetrieveResources	Get resources from given list
//	StopDevice			Stop device
//*	WriteByte			Output a byte
//*	ReadByte			Input a byte
//	WdmIoStartIo		Process an IRP from the head of the device IRP queue.
//*	StoreCmds			Copy commands from IOCTL input buffer to new buffer (in dx)
//*	RunCmds				Run commands for IOCTL_WDMIO_RUN_CMDS
//*	RunCmdsSynch		RunCmds called as a Critical Section routine
//*	RunWriteCmdsSynch	Run stored write commands
//*	RunReadCmdsSynch	Run stored read commands
//*	RunStartReadCmdsSynch	Run stored start read commands
//*	ProcessCmds			Process commands in given buffer.
//*	IrqConnectRoutine	Work queue item to connect to an interrupt at PASSIVE_LEVEL
//*	InterruptHandler	Handle interrupts (during StartIo processing of R/W)
//	WdmIoDpcForIsr		Complete current IRP
//*	Timeout1sSynch		Timeout check 
//	Timeout1s			One second timer call
//	WdmIoCancelIrp		Cancel this IRP
//*	CancelCurrentIrpSynch	If a transfer is in progress, mark it for cancelling
//	WdmIoCleanup		Handle IRP_MJ_CLEANUP requests
/////////////////////////////////////////////////////////////////////////////
//	Version history
//	14-May-99	1.0.0	CC	creation
/////////////////////////////////////////////////////////////////////////////

#include "wdmIo.h"
#include "Ioctl.h"

/////////////////////////////////////////////////////////////////////////////

NTSTATUS StoreCmds( PUCHAR* pCmds, ULONG* pCmdsLen, ULONG len, PVOID Buffer);
NTSTATUS RetrieveResources(IN PWDMIO_DEVICE_EXTENSION dx,
						   IN PCM_RESOURCE_LIST AllocatedResourcesTranslated);
void WriteByte( IN PWDMIO_DEVICE_EXTENSION dx, IN ULONG offset, IN UCHAR byte);
UCHAR ReadByte( IN PWDMIO_DEVICE_EXTENSION dx, IN ULONG offset);
BOOLEAN InterruptHandler(IN PKINTERRUPT Interrupt, IN PWDMIO_DEVICE_EXTENSION dx);

BOOLEAN RunCmdsSynch( IN PDEVICE_OBJECT fdo);
BOOLEAN RunCmds( IN PDEVICE_OBJECT fdo, IN bool CanTrace);
bool ProcessCmds(	IN PWDMIO_DEVICE_EXTENSION dx,
					IN PUCHAR Buffer, IN ULONG len,
					OUT PUCHAR OutBuffer, IN ULONG outlen,
					IN bool CanTrace
				  );

BOOLEAN RunWriteCmdsSynch( IN PWDMIO_DEVICE_EXTENSION dx);
BOOLEAN RunReadCmdsSynch( IN PWDMIO_DEVICE_EXTENSION dx);
BOOLEAN RunStartReadCmdsSynch( IN PWDMIO_DEVICE_EXTENSION dx);

/////////////////////////////////////////////////////////////////////////////
//	StartDevice:	Start the device

NTSTATUS StartDevice( IN PWDMIO_DEVICE_EXTENSION dx, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	if( dx->GotResources)
		return STATUS_SUCCESS;

	NTSTATUS status = RetrieveResources(dx,AllocatedResourcesTranslated);
	if( !NT_SUCCESS(status))
		return status;

	// Map memory
	if( dx->PortNeedsMapping)
	{
		dx->PortBase = (PUCHAR)MmMapIoSpace( dx->PortStartAddress, dx->PortLength, MmNonCached);
		if( !dx->PortBase)
			return STATUS_NO_MEMORY;
	}
	else
		dx->PortBase = (PUCHAR)dx->PortStartAddress.LowPart;

	// Reconnect to interrupt if necessary
	if( dx->ConnectedToInterrupt)
	{
		if( !dx->GotInterrupt)
			status = STATUS_INSUFFICIENT_RESOURCES;
		else
			status = IoConnectInterrupt( &dx->InterruptObject, (PKSERVICE_ROUTINE)InterruptHandler,
							dx, NULL, dx->Vector, dx->Irql, dx->Irql, dx->Mode, FALSE, dx->Affinity, FALSE);
		if( !NT_SUCCESS(status))
		{
			dx->ConnectedToInterrupt = false;
			return status;
		}
	}

	// Device is now started
	dx->GotResources = true;
	
	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//	RetrieveResources:	Get resources from given list.
//						Must at least have a port or memory given.
//						Save any given interrupt as well.

NTSTATUS RetrieveResources( IN PWDMIO_DEVICE_EXTENSION dx, IN PCM_RESOURCE_LIST AllocatedResourcesTranslated)
{
	if( AllocatedResourcesTranslated==NULL ||
		AllocatedResourcesTranslated->Count==0)
	{
		DebugPrintMsg("RetrieveResources: No allocated translated resources");
		return STATUS_DEVICE_CONFIGURATION_ERROR;
	}

	// Get to actual resources
	PCM_PARTIAL_RESOURCE_LIST list = &AllocatedResourcesTranslated->List[0].PartialResourceList;
	PCM_PARTIAL_RESOURCE_DESCRIPTOR resource = list->PartialDescriptors;
	ULONG NumResources = list->Count;

	DebugPrint("RetrieveResources: %d resource lists %d resources", AllocatedResourcesTranslated->Count, NumResources);

	bool GotError = false;

	// Clear dx
	dx->GotInterrupt = false;
	dx->GotPortOrMemory = false;

	// Go through each allocated resource
	for( ULONG resno=0; resno<NumResources; resno++,resource++)
	{
		switch( resource->Type)
		{
		case CmResourceTypePort:
			if( dx->GotPortOrMemory) { GotError = true; break; }
			dx->GotPortOrMemory = true;
			dx->PortStartAddress = resource->u.Port.Start;
			dx->PortLength = resource->u.Port.Length;
			dx->PortNeedsMapping = (resource->Flags & CM_RESOURCE_PORT_IO)==0;
			dx->PortInIOSpace = !dx->PortNeedsMapping;
			DebugPrint("RetrieveResources: Port %L Length %d NeedsMapping %d",
							dx->PortStartAddress,
							dx->PortLength, dx->PortNeedsMapping);
			break;

		case CmResourceTypeInterrupt:
			dx->GotInterrupt = true;
			dx->Irql = (KIRQL)resource->u.Interrupt.Level;
			dx->Vector = resource->u.Interrupt.Vector;
			dx->Affinity = resource->u.Interrupt.Affinity;
			dx->Mode = (resource->Flags == CM_RESOURCE_INTERRUPT_LATCHED)
						? Latched : LevelSensitive;
			DebugPrint("RetrieveResources: Interrupt vector %x IRQL %d Affinity %d Mode %d",
							dx->Vector, dx->Irql, dx->Affinity, dx->Mode);
			break;

		case CmResourceTypeMemory:
			if( dx->GotPortOrMemory) { GotError = true; break; }
			dx->GotPortOrMemory = true;
			dx->PortStartAddress = resource->u.Memory.Start;
			dx->PortLength = resource->u.Memory.Length;
			dx->PortInIOSpace = false;
			dx->PortNeedsMapping = true;
			DebugPrint("RetrieveResources: Memory %L Length %d",
							dx->PortStartAddress, dx->PortLength);
			break;

		case CmResourceTypeDma:
		case CmResourceTypeDeviceSpecific:
		case CmResourceTypeBusNumber:
		default:
			DebugPrint("RetrieveResources: Unrecognised resource type %d", resource->Type);
			GotError = true;
			break;
		}
	}

	// Check we've got the resources we need
	if( GotError || !dx->GotPortOrMemory /*|| !GotInterrupt*/)
		return STATUS_DEVICE_CONFIGURATION_ERROR;

	return STATUS_SUCCESS;
}
	
/////////////////////////////////////////////////////////////////////////////
//	StopDevice:	Stop device

VOID StopDevice( IN PWDMIO_DEVICE_EXTENSION dx)
{
	DebugPrintMsg("StopDevice");
	if( !dx->GotResources)
		return;
	dx->GotResources = false;

	// Unmap memory
	if (dx->PortNeedsMapping)
		MmUnmapIoSpace( (PVOID)dx->PortBase, dx->PortLength);

	// Disconnect from interrupt
	if( dx->ConnectedToInterrupt)
	{
		IoDisconnectInterrupt( dx->InterruptObject);
		//dx->ConnectedToInterrupt = false;	Don't do this.  So we know to reconnect
	}
}

/////////////////////////////////////////////////////////////////////////////
//	WriteByte:	Output a byte
//				Silently ignores request if register out of range
//	Don't call DebugPrint as may be called at DIRQL

void WriteByte( IN PWDMIO_DEVICE_EXTENSION dx, IN ULONG offset, IN UCHAR byte)
{
	if( offset>=dx->PortLength) return;
	PUCHAR Port = dx->PortBase+offset;
	if( dx->PortInIOSpace)
		WRITE_PORT_UCHAR( Port, byte);
	else
		WRITE_REGISTER_UCHAR( Port, byte);
}

/////////////////////////////////////////////////////////////////////////////
//	ReadByte:	Input a byte
//				Silently ignores request if register out of range
//	Don't call DebugPrint as may be called at DIRQL

UCHAR ReadByte( IN PWDMIO_DEVICE_EXTENSION dx, IN ULONG offset)
{
	if( offset>=dx->PortLength) return 0;
	PUCHAR Port = dx->PortBase+offset;
	UCHAR b;
	if( dx->PortInIOSpace)
		b = READ_PORT_UCHAR(Port);
	else
		b = READ_REGISTER_UCHAR(Port);
	return b;
}

/////////////////////////////////////////////////////////////////////////////
//	WdmIoStartIo:	Process an IRP from the head of the device IRP queue.
//				1	Only IOCTL, Read and Write IRPs get here.
//				2	The IRP is either completed here, or completed once the
//					interrupt driven read or writes completes, times out
//					or is cancelled.
//				3	Note that IRP may be cancelled at any time during this
//					processing, so we check IRP's Cancel flag when appropriate.
//				4	The Irp parameter is equal to fdo->CurrentIrp until it
//					is completed and IoStartNextPacket called.

VOID WdmIoStartIo( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	PUCHAR Buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

	// Zero the output count
	dx->CmdOutputCount = 0;
	dx->ConnectIntQueued = false;

	DebugPrint( "WdmIoStartIo: %x %I",Irp,Irp);

	// Stop the 1 second timer if necessary
	if( dx->StopTimer)
	{
		IoStopTimer(fdo);
		dx->StopTimer = false;
	}

	NTSTATUS status = STATUS_SUCCESS;

	// Switch on the IRP major function code
	switch( IrpStack->MajorFunction)
	{
	/////////////////////////////////////////////////////////////////////////
	case IRP_MJ_DEVICE_CONTROL:
	{
		ULONG ControlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
		ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
		ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
		switch( ControlCode)
		{
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case IOCTL_PHDIO_RUN_CMDS:
			DebugPrint( "WdmIoStartIo: Run Cmds %s", dx->ConnectedToInterrupt?"(synchronised)":"");
			// If necessary make a temp buffer for the output data
			dx->RunCmdsOutBuffer = NULL;
			if( OutputLength>0)
			{
				dx->RunCmdsOutBuffer = (PUCHAR)ExAllocatePool(NonPagedPool,OutputLength);
				if( dx->RunCmdsOutBuffer==NULL)
				{
					status = STATUS_UNSUCCESSFUL;
					break;
				}
			}
			// Run the commands, synchronized with interrupt if necessary
			if( dx->ConnectedToInterrupt)
			{
				if( !KeSynchronizeExecution( dx->InterruptObject, (PKSYNCHRONIZE_ROUTINE)RunCmdsSynch, (PVOID)fdo))
					status = STATUS_UNSUCCESSFUL;
			}
			else
			{
				if( !RunCmds(fdo,true))
					status = STATUS_UNSUCCESSFUL;

				// Return straightaway if ConnectIntWQI queued
				if( dx->ConnectIntQueued) return;
			}
			// Copy temp output buffer back into shared IOCTL buffer
			if( dx->RunCmdsOutBuffer!=NULL)
			{
				RtlCopyMemory( Buffer, dx->RunCmdsOutBuffer, OutputLength);
				ExFreePool(dx->RunCmdsOutBuffer);
				dx->RunCmdsOutBuffer = NULL;
			}
			break;
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case IOCTL_PHDIO_CMDS_FOR_READ:
			DebugPrintMsg( "WdmIoStartIo: Store cmds for read");
			status = StoreCmds( &dx->ReadCmds, &dx->ReadCmdsLen, InputLength, Buffer);
			break;
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case IOCTL_PHDIO_CMDS_FOR_READ_START:
			DebugPrintMsg( "WdmIoStartIo: Store cmds for read start");
			status = StoreCmds( &dx->StartReadCmds, &dx->StartReadCmdsLen, InputLength, Buffer);
			break;
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case IOCTL_PHDIO_CMDS_FOR_WRITE:
			DebugPrintMsg( "WdmIoStartIo: Store cmds for write");
			status = StoreCmds( &dx->WriteCmds, &dx->WriteCmdsLen, InputLength, Buffer);
			break;
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case IOCTL_PHDIO_GET_RW_RESULTS:
#if DBG
			if( dx->TxCmdOutputCount>sizeof(dx->TxResult))
			{
				DebugPrint( "WdmIoStartIo: Get RW Results: dx->TxCmdOutputCount too big at %d",dx->CmdOutputCount);
				dx->CmdOutputCount = sizeof(dx->TxResult);
			}
#endif
			// Copy cmd output first
			dx->CmdOutputCount = dx->TxCmdOutputCount;
			if( dx->CmdOutputCount>OutputLength)
				 dx->CmdOutputCount = OutputLength;
			RtlCopyMemory( Buffer, dx->TxResult, dx->CmdOutputCount);
			// Then add on last interrupt reg value
			if( dx->CmdOutputCount+1<=OutputLength)
				Buffer[dx->CmdOutputCount++] = dx->TxLastIntReg;

			DebugPrint( "WdmIoStartIo: Get RW Results: %d bytes",dx->CmdOutputCount);
			break;
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		default:
			status = STATUS_NOT_SUPPORTED;
		}
		break;
	}

	/////////////////////////////////////////////////////////////////////////
	case IRP_MJ_WRITE:
		if( dx->WriteCmds==NULL || !dx->ConnectedToInterrupt)
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

		// Store transfer details
		dx->TxTotal = IrpStack->Parameters.Write.Length;
		dx->TxLeft = dx->TxTotal;
		dx->TxBuffer = (PUCHAR)Buffer;
		dx->TxStatus = STATUS_SUCCESS;
		dx->TxIsWrite = true;
		RtlZeroMemory( dx->TxResult, sizeof(dx->TxResult));
		DebugPrint( "WdmIoStartIo: Write %d bytes: %*s",dx->TxTotal,dx->TxTotal,dx->TxBuffer);

		// Start timeout timer
		dx->Timeout = dx->SetTimeout+1;
		IoStartTimer(fdo);

		// Send first value
		if( KeSynchronizeExecution( dx->InterruptObject, (PKSYNCHRONIZE_ROUTINE)RunWriteCmdsSynch, (PVOID)dx))
		{
			status = STATUS_UNSUCCESSFUL;
			break;
		}
		return;

	/////////////////////////////////////////////////////////////////////////
	case IRP_MJ_READ:
		if( dx->ReadCmds==NULL || !dx->ConnectedToInterrupt)
		{
			status = STATUS_INVALID_DEVICE_REQUEST;
			break;
		}

		// Store transfer details
		dx->TxTotal = IrpStack->Parameters.Read.Length;
		dx->TxLeft = dx->TxTotal;
		dx->TxBuffer = (PUCHAR)Buffer;
		dx->TxStatus = STATUS_SUCCESS;
		dx->TxIsWrite = false;
		RtlZeroMemory( dx->TxResult, sizeof(dx->TxResult));
		DebugPrint( "WdmIoStartIo: Read %d bytes: %*s",dx->TxTotal,dx->TxTotal,dx->TxBuffer);

		// Start timeout timer
		dx->Timeout = dx->SetTimeout;
		if( dx->Timeout<=0) dx->Timeout = 10;
		IoStartTimer(fdo);

		// Run StartReadCmds if available
		if( dx->StartReadCmds!=NULL)
		{
			DebugPrintMsg( "WdmIoStartIo: Read: Running start read commands");
			if( KeSynchronizeExecution( dx->InterruptObject, (PKSYNCHRONIZE_ROUTINE)RunStartReadCmdsSynch, (PVOID)dx))
			{
				status = STATUS_UNSUCCESSFUL;
				break;
			}
		}
		return;
	/////////////////////////////////////////////////////////////////////////
	default:
		status = STATUS_NOT_SUPPORTED;
		break;
	}

	/////////////////////////////////////////////////////////////////////////
	// Complete this IRP

	if( Irp->Cancel) status = STATUS_CANCELLED;

	// Remove cancel routine
	KIRQL OldIrql;
	IoAcquireCancelSpinLock( &OldIrql);
	IoSetCancelRoutine( Irp, NULL);
	IoReleaseCancelSpinLock(OldIrql);

	// Unlock device, complete IRP and start next
	UnlockDevice(dx);
	DebugPrint( "WdmIoStartIo: CmdOutputCount %d", dx->CmdOutputCount);
	CompleteIrp(Irp, status, dx->CmdOutputCount);
	IoStartNextPacket( fdo, TRUE);
}

/////////////////////////////////////////////////////////////////////////////
//	StoreCmds:	Copy commands from IOCTL input buffer to new buffer (in dx)

NTSTATUS StoreCmds( PUCHAR* pCmds, ULONG* pCmdsLen, ULONG len, PVOID Buffer)
{
	// Save commands for later processing
	if( len==0) return STATUS_INVALID_PARAMETER;
	FreeIfAllocated(*pCmds);
	*pCmds = (PUCHAR)ExAllocatePool( NonPagedPool, len);
	if( *pCmds==NULL)
		return STATUS_NO_MEMORY;
	RtlCopyMemory( *pCmds, Buffer, len);
	*pCmdsLen = len;
	return STATUS_SUCCESS;
}

/////////////////////////////////////////////////////////////////////////////
//	RunCmds:		Run commands for IOCTL_WDMIO_RUN_CMDS
//	RunCmdsSynch:	RunCmds called as a Critical Section routine at DIRQL
//
//	Only do trace output if not run as a Critical Section routine.
//
//	Runs at DISPATCH_LEVEL or DIRQL
//	Return	TRUE if commands ran (successfully or not)

BOOLEAN RunCmdsSynch( IN PDEVICE_OBJECT fdo)
{
	return RunCmds( fdo, false);
}

BOOLEAN RunCmds( IN PDEVICE_OBJECT fdo, IN bool CanTrace)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	PIRP Irp = fdo->CurrentIrp;
	PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
	ULONG InputLength = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
	PUCHAR Buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;

	ProcessCmds( dx, Buffer, InputLength, dx->RunCmdsOutBuffer, OutputLength, CanTrace);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	RunWriteCmdsSynch:	Run stored write commands
//						ProcessCmds output sent to dx->TxResult
//	Runs at DIRQL
//	Return TRUE if bytes all transferred (or in error)

BOOLEAN RunWriteCmdsSynch( IN PWDMIO_DEVICE_EXTENSION dx)
{
	if( dx->TxLeft==0) return TRUE;

	dx->CmdOutputCount = 0;
	BOOLEAN rv = ProcessCmds( dx, dx->WriteCmds, dx->WriteCmdsLen, dx->TxResult, sizeof(dx->TxResult), false);
	dx->TxCmdOutputCount = dx->CmdOutputCount;
	if( !rv)
	{
		dx->TxStatus = STATUS_UNSUCCESSFUL;
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//	RunReadCmdsSynch:	Run stored read commands
//						ProcessCmds output sent to dx->TxResult
//	Runs at DIRQL
//	Return TRUE if bytes all transferred (or in error)

BOOLEAN RunReadCmdsSynch( IN PWDMIO_DEVICE_EXTENSION dx)
{
	if( dx->TxLeft==0) return TRUE;

	dx->CmdOutputCount = 0;
	BOOLEAN rv = ProcessCmds( dx, dx->ReadCmds, dx->ReadCmdsLen, dx->TxResult, sizeof(dx->TxResult), false);
	dx->TxCmdOutputCount = dx->CmdOutputCount;
	if( !rv)
	{
		dx->TxStatus = STATUS_UNSUCCESSFUL;
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//	RunStartReadCmdsSynch:	Run stored start read commands
//							ProcessCmds output sent to dx->TxResult
//	Runs at DIRQL
//	Return TRUE if bytes all transferred (or in error)

BOOLEAN RunStartReadCmdsSynch( IN PWDMIO_DEVICE_EXTENSION dx)
{
	if( dx->TxLeft==0) return TRUE;

	dx->CmdOutputCount = 0;
	BOOLEAN rv = ProcessCmds( dx, dx->StartReadCmds, dx->StartReadCmdsLen, dx->TxResult, sizeof(dx->TxResult), false);
	dx->TxCmdOutputCount = dx->CmdOutputCount;
	if( !rv)
	{
		dx->TxStatus = STATUS_UNSUCCESSFUL;
		return TRUE;
	}
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//	Command names for DebugPrint

#if DODEBUGPRINT
static char* PHD_IO_CMDS[] =
{
	"PHDIO_OR",
	"PHDIO_AND",
	"PHDIO_XOR",

	"PHDIO_WRITE",
	"PHDIO_READ",

	"PHDIO_DELAY",

	"PHDIO_WRITES",
	"PHDIO_READS",

	"PHDIO_IRQ_CONNECT",

	"PHDIO_TIMEOUT",
	"PHDIO_WRITE_NEXT",
	"PHDIO_READ_NEXT",
};
static int NUM_PHD_IO_CMDS = sizeof(PHD_IO_CMDS)/sizeof(char*);
#endif

/////////////////////////////////////////////////////////////////////////////
//	Useful macros for ProcessCmds
//	GetUChar:			Get next UCHAR in newly declared variable (if available)
//	GetUCharNoDeclare:	Get next UCHAR in existing variable (if available)
//	SetUChar:			Store UCHAR in output buffer (if there's room)

#define GetUChar(var) if( ByteNo>=len) { FailCode=PHDIO_NO_CMD_PARAMS; goto fail; } UCHAR var = *Buffer++; ByteNo++
#define GetUCharNoDeclare(var) if( ByteNo>=len) { FailCode=PHDIO_NO_CMD_PARAMS; goto fail; }; var = *Buffer++; ByteNo++
#define SetUChar(b) if( OutByteNo>=outlen) { FailCode=PHDIO_NO_OUTPUT_ROOM; goto fail; } *OutBuffer++ = (b); OutByteNo++; dx->CmdOutputCount++

/////////////////////////////////////////////////////////////////////////////
//	ProcessCmds:	Process commands in given buffer.
//					If output buffer given, first word has result code and
//					second word has index into input buffer of problem cmd.
//					Actual output values are stored after this.
//
//	Currently can only process UCHAR size commands.
//	Only produce DebugPrint output if CanTrace is true
//
//	return false if there's a problem

bool ProcessCmds(	IN PWDMIO_DEVICE_EXTENSION dx,
					IN PUCHAR Buffer, IN ULONG len,
					OUT PUCHAR OutBuffer, IN ULONG outlen,
					IN bool CanTrace
				  )
{
	if( CanTrace) { DebugPrint( "ProcessCmds. input:%d output:%d", len, outlen); }

	PIRP Irp = dx->fdo->CurrentIrp;

	// Zero first 2 words of output buffer (if available)
	PUSHORT Result = (PUSHORT)OutBuffer;
	const int ResultLen = 2*sizeof(USHORT);
	if( outlen<ResultLen)
	{
		Result = NULL;
		outlen = 0;
	}
	else
	{
		OutBuffer += ResultLen;
		outlen -= ResultLen;
		*Result = PHDIO_OK;
		*(Result+1) = 0;
		dx->CmdOutputCount += ResultLen;
	}
	USHORT FailCode = PHDIO_OK;

	USHORT ByteNo=0;
	USHORT OutByteNo=0;
	// Loop through all commands
	while( ByteNo<len)
	{
		// See if we've been cancelled
		if( Irp->Cancel)
		{
			FailCode = PHDIO_CANCELLED;
			goto fail;
		}
		// Get next command
		UCHAR Cmd = *Buffer++;
		ByteNo++;
		UCHAR Size = Cmd&0xC0;
		if( Size!=0)
		{
			FailCode = PHDIO_BYTE_CMDS_ONLY;
			goto fail;	// Replace with following once uwords and ulongs supported
		}
/*
		bool isUCHAR = true;
		bool isUWORD = false;
		bool isULONG = false;
		if( Size==0x40) { isUCHAR = false; isUWORD = true; }
		if( Size==0x80) { isUCHAR = false; isULONG = true; }
		Cmd &= 0x3F;
*/
#if DODEBUGPRINT
		if( CanTrace)
			if( Cmd<NUM_PHD_IO_CMDS)
				DebugPrint( "Cmd: %s", PHD_IO_CMDS[Cmd]);
#endif
		/////////////////////////////////////////////////////////////////////
		switch( Cmd)
		{
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_OR:
		{
			GetUChar(reg);
			GetUChar(orvalue);
			UCHAR bvalue = ReadByte( dx, reg);
			UCHAR oredvalue = bvalue|orvalue;
			if( CanTrace) { DebugPrint( "Or %d %2x.  %2x->%2x", reg, orvalue, bvalue, oredvalue); }
			WriteByte( dx, reg, oredvalue);
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_AND:
		{
			GetUChar(reg);
			GetUChar(andvalue);
			UCHAR bvalue = ReadByte( dx, reg);
			UCHAR andedvalue = bvalue&andvalue;
			if( CanTrace) { DebugPrint( "And %d %2x.  %2x->%2x", reg, andvalue, bvalue, andedvalue); }
			WriteByte( dx, reg, andedvalue);
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_XOR:
		{
			GetUChar(reg);
			GetUChar(xorvalue);
			UCHAR bvalue = ReadByte( dx, reg);
			UCHAR xoredvalue = bvalue^xorvalue;
			if( CanTrace) { DebugPrint( "Xor %d %2x.  %2x->%2x", reg, xorvalue, bvalue, xoredvalue); }
			WriteByte( dx, reg, xoredvalue);
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_WRITE:
		{
			GetUChar(reg);
			GetUChar(bvalue);
			if( CanTrace) { DebugPrint( "Write %d %2x", reg, bvalue); }
			WriteByte( dx, reg, bvalue);
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_READ:
		{
			GetUChar(reg);
			UCHAR bvalue = ReadByte( dx, reg);
			SetUChar(bvalue);
			if( CanTrace) { DebugPrint( "Read %d %2x", reg, bvalue); }
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_DELAY:
		{
			GetUChar(delay);
			if( CanTrace) { DebugPrint( "Delay %dus", delay); }
			if( delay>60) { FailCode = PHDIO_DELAY_TOO_LONG; goto fail; }
			KeStallExecutionProcessor(delay);
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_WRITES:
		{
			GetUChar(reg);
			GetUChar(count);
			GetUChar(delay);
			if( CanTrace) { DebugPrint( "Write %d values to %d, delay %dus", count, reg, delay); }
			if( delay>60) { FailCode = PHDIO_DELAY_TOO_LONG; goto fail; }
			for( UCHAR vno=0; vno<count; vno++)
			{
				GetUChar(bvalue);
				if( CanTrace) { DebugPrint( "Writing %d %2x", reg, bvalue); }
				WriteByte( dx, reg, bvalue);
				KeStallExecutionProcessor(delay);
			}
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_READS:
		{
			GetUChar(reg);
			GetUChar(count);
			GetUChar(delay);
			if( CanTrace) { DebugPrint( "Read %d values from %d, delay %dus", count, reg, delay); }
			if( delay>60) { FailCode = PHDIO_DELAY_TOO_LONG; goto fail; }
			for( UCHAR vno=0; vno<count; vno++)
			{
				UCHAR bvalue = ReadByte( dx, reg);
				KeStallExecutionProcessor(delay);
				SetUChar(bvalue);
				if( CanTrace) { DebugPrint( "Read %d %2x", reg, bvalue); }
			}
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_IRQ_CONNECT:
		{
			if( !dx->GotInterrupt) { FailCode = PHDIO_NO_INTERRUPT; goto fail; }
			GetUCharNoDeclare(dx->InterruptReg);
			if( dx->InterruptReg>=dx->PortLength) { FailCode = PHDIO_NOT_IN_RANGE; goto fail; }
			GetUCharNoDeclare(dx->InterruptRegMask);
			GetUCharNoDeclare(dx->InterruptRegValue);
			if( (dx->InterruptRegValue&dx->InterruptRegMask) != dx->InterruptRegValue)
				{ FailCode = PHDIO_BAD_INTERRUPT_VALUE; goto fail; }

			if( CanTrace) { DebugPrint( "Connect.  Reg %d  Mask %2x Value %2x",
				dx->InterruptReg, dx->InterruptRegMask, dx->InterruptRegValue); }

			dx->ConnectIntQueued = true;
			ExQueueWorkItem( &dx->ConnectIntWQI, DelayedWorkQueue);

			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_TIMEOUT:
			GetUCharNoDeclare(dx->SetTimeout);
			break;
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_WRITE_NEXT:
		{
			if( dx->Timeout==-1) { FailCode = PHDIO_CANNOT_RW_NEXT; goto fail; }
			if( dx->TxLeft==0) { FailCode = PHDIO_NO_DATA_LEFT_TO_TRANSFER; goto fail; }
			GetUChar(reg);
			WriteByte( dx, reg, *dx->TxBuffer++);
			dx->TxLeft--;
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		case PHDIO_READ_NEXT:
		{
			if( dx->Timeout==-1) { FailCode = PHDIO_CANNOT_RW_NEXT; goto fail; }
			if( dx->TxLeft==0) { FailCode = PHDIO_NO_DATA_LEFT_TO_TRANSFER; goto fail; }
			GetUChar(reg);
			*dx->TxBuffer++ = ReadByte( dx, reg);
			dx->TxLeft--;
			break;
		}
		// / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / / //
		default:
			FailCode = PHDIO_UNRECOGNISED_CMD;
			goto fail;
		}
	}
	return true;

	/////////////////////////////////////////////////////////////////////////
	// Store failure code and location
fail:
	if( CanTrace) { DebugPrint( "ProcessCmds. FailCode %d at input:%d output:%d", FailCode, ByteNo-1, OutByteNo); }
	if( Result!=NULL)
	{
		*Result++ = FailCode;
		*Result = ByteNo-1;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////
//	IrqConnectRoutine:	Work queue item to connect to an interrupt at PASSIVE_LEVEL

VOID IrqConnectRoutine( IN PVOID Context)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)Context;

	DebugPrint( "IrqConnectRoutine");
	dx->ConnectIntQueued = false;

	// Get the current IRP that we are working on
	PIRP Irp = dx->fdo->CurrentIrp;
	if( Irp==NULL) return;

	// Cancel if necessary
	NTSTATUS status;
	if( Irp->Cancel)
		status = STATUS_CANCELLED;
	else
	{
		// Try to connect to interrupt
		status = IoConnectInterrupt( &dx->InterruptObject, (PKSERVICE_ROUTINE)InterruptHandler,
							dx, NULL, dx->Vector, dx->Irql, dx->Irql, dx->Mode, FALSE, dx->Affinity, FALSE);
		if( NT_SUCCESS(status))
			dx->ConnectedToInterrupt = true;
		else
		{
			// Store FailCode in output 
			if( dx->RunCmdsOutBuffer!=NULL)
				*dx->RunCmdsOutBuffer = PHDIO_CANNOT_CONNECT_TO_INTERRUPT;
		}
	}

	// Copy commands output to RUN_CMDS buffer
	if( dx->RunCmdsOutBuffer!=NULL)
	{
		PUCHAR Buffer = (PUCHAR)Irp->AssociatedIrp.SystemBuffer;
		PIO_STACK_LOCATION IrpStack = IoGetCurrentIrpStackLocation(Irp);
		ULONG OutputLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;

		RtlCopyMemory( Buffer, dx->RunCmdsOutBuffer, OutputLength);
		ExFreePool(dx->RunCmdsOutBuffer);
		dx->RunCmdsOutBuffer = NULL;
	}

	// Remove cancel routine
	KIRQL OldIrql;
	IoAcquireCancelSpinLock( &OldIrql);
	IoSetCancelRoutine( Irp, NULL);
	IoReleaseCancelSpinLock(OldIrql);

	// Unlock device, complete IRP and start next
	UnlockDevice(dx);
	DebugPrint( "IrqConnectRoutine: CmdOutputCount %d", dx->CmdOutputCount);
	CompleteIrp(Irp, status, dx->CmdOutputCount);
	KeRaiseIrql( DISPATCH_LEVEL, &OldIrql);
	IoStartNextPacket( dx->fdo, TRUE);
	KeLowerIrql(OldIrql);
}

/////////////////////////////////////////////////////////////////////////////
//	InterruptHandler:	Handle interrupts (during StartIo processing of R/W)
//					1	Always read the relevant status regsiter.
//					2	Only proceed if it has the right value,
//						ie to signal that our device caused the interrupt.
//					3	If IRP being cancelled then just call DPC to complete IRP.
//					4	Normally run write or read cmds to do whatever is necessary
//						to output or input next byte.
//					5	If all buffer txd (or error) then call DPC to complete IRP
//	Do not call DebugPrint here
//	Return TRUE if interrupt handled

BOOLEAN InterruptHandler(IN PKINTERRUPT Interrupt, IN PWDMIO_DEVICE_EXTENSION dx)
{
	// See if interrupt is ours
	dx->TxLastIntReg = ReadByte( dx, dx->InterruptReg);
	if( (dx->TxLastIntReg&dx->InterruptRegMask) != dx->InterruptRegValue)
		return FALSE;

	// If no transfer in progress then no further processing required
	if( dx->Timeout==-1) return TRUE;

	// See if current IRP being cancelled
	PDEVICE_OBJECT fdo = dx->fdo;
	PIRP Irp = fdo->CurrentIrp;
	if( Irp==NULL) return TRUE;
	BOOLEAN TxComplete = Irp->Cancel;
	if( !TxComplete)
	{
		// Run relevant set of commands
		if( dx->TxIsWrite)
			TxComplete = RunWriteCmdsSynch(dx);
		else
			TxComplete = RunReadCmdsSynch(dx);
	}
	// If all done, in error or being cancelled then call DPC to complete IRP
	if( TxComplete)
	{
		dx->Timeout = -1;
		IoRequestDpc( fdo, Irp, dx);
	}
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	WdmIoDpcForIsr:	Complete current IRP

VOID WdmIoDpcForIsr(IN PKDPC Dpc, IN PDEVICE_OBJECT fdo, 
					IN PIRP Irp, IN PWDMIO_DEVICE_EXTENSION dx)
{
	dx->Timeout = -1;
	ULONG BytesTxd = dx->TxTotal-dx->TxLeft;
	if( Irp->Cancel) dx->TxStatus = STATUS_CANCELLED;

	DebugPrint("WdmIoDpcForIsr: Status %x Info %d", dx->TxStatus, BytesTxd);

	// Remove cancel routine
	KIRQL OldIrql;
	IoAcquireCancelSpinLock( &OldIrql);
	IoSetCancelRoutine( Irp, NULL);
	IoReleaseCancelSpinLock(OldIrql);

	// Unlock device and complete IRP
	UnlockDevice(dx);
	CompleteIrp(Irp, dx->TxStatus, BytesTxd);
	IoStartNextPacket( fdo, TRUE);

	// Stop timer calls
	dx->StopTimer = true;
}

/////////////////////////////////////////////////////////////////////////////
//	Timeout1sSynch:	Timeout check 
//	Return TRUE if operation has timed out
//	Called as a Critical Section routine

static BOOLEAN Timeout1sSynch( IN PWDMIO_DEVICE_EXTENSION dx)
{
	if( dx->Timeout==-1 || --dx->Timeout>0)
		return FALSE;
	dx->Timeout = -1;
	dx->TxStatus = STATUS_NO_MEDIA_IN_DEVICE;	// Win32: ERROR_NOT_READY
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//	Timeout1s:	One second timer call
//				Call Timeout1sSynch and then DPC if time out

VOID Timeout1s( IN PDEVICE_OBJECT fdo, IN PWDMIO_DEVICE_EXTENSION dx)
{
	if( dx->Timeout==-1) return;

	DebugPrint("Timeout1s: Timeout is %d",dx->Timeout);
	PIRP Irp = fdo->CurrentIrp;
#if DBG
	if( Irp==NULL) return;
#endif
	if( Irp->Cancel || KeSynchronizeExecution( dx->InterruptObject, (PKSYNCHRONIZE_ROUTINE)Timeout1sSynch, dx))
		WdmIoDpcForIsr( NULL, fdo, fdo->CurrentIrp, dx);
}

/////////////////////////////////////////////////////////////////////////////
//	WdmIoCancelIrp:
//
//	Description:
//		Cancel this IRP.
//			Called to cancel a Irp.
//			Called when CancelIo called or process finished without closing handle.
//			IRP must have set this as its cancel routine.
//
//		1	If IRP currently being processed in StartIo or interrupt handler
//			then just quit without completing IRP.  The IRP Cancel flag will 
//			be detected in due course and the IRP completed (as cancelled) then.
//		2	If IRP still in StartIo queue then remove it and complete it as cancelled.

VOID WdmIoCancelIrp( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrint("WdmIoCancelIrp: Cancelling %x %I", Irp, Irp);
	if( Irp==fdo->CurrentIrp)
	{
		DebugPrintMsg("WdmIoCancelIrp: IRP running in StartIo");
		// IRP is being processed by WdmIoStartIo.
		// Irp->Cancel flag already set.
		// WdmIoStartIo or timeout will detect Cancel flag and cancel IRP in due course
		IoReleaseCancelSpinLock(Irp->CancelIrql);
	}
	else
	{
		DebugPrintMsg("WdmIoCancelIrp: IRP in StartIo queue");
		// IRP is still in StartIo device queue.
		// Just dequeue and cancel it.  No need to start next IRP.
		BOOLEAN dequeued = KeRemoveEntryDeviceQueue(
								&fdo->DeviceQueue,
								&Irp->Tail.Overlay.DeviceQueueEntry);

		IoReleaseCancelSpinLock(Irp->CancelIrql);

		if( dequeued)
		{
			UnlockDevice(dx);
			CompleteIrp( Irp, STATUS_CANCELLED);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////
//	CancelCurrentIrpSynch:	If a transfer is in progress, mark it for cancelling
//	Return TRUE if operation can been cancelled
//	Runs at DIRQL

static BOOLEAN CancelCurrentIrpSynch( IN PWDMIO_DEVICE_EXTENSION dx)
{
	if( dx->Timeout==-1)
		return FALSE;
	dx->Timeout = -1;
	dx->TxStatus = STATUS_CANCELLED;
	return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//	WdmIoCleanup:
//
//	Description:
//		Handle IRP_MJ_CLEANUP requests
//		Cancel queued IRPs which match given FileObject
//
//		WdmIo cancels *all* queued IRPs and the current Irp
//
//	Arguments:
//		Pointer to our FDO
//		Pointer to the IRP
//			IrpStack->FileObject has handle to file
//
//	Return Value:
//		This function returns STATUS_XXX

NTSTATUS WdmIoDispatchCleanup( IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
	PWDMIO_DEVICE_EXTENSION dx = (PWDMIO_DEVICE_EXTENSION)fdo->DeviceExtension;
	DebugPrintMsg("WdmIoDispatchCleanup");
	KIRQL OldIrql;
	IoAcquireCancelSpinLock(&OldIrql);

	// Cancel all IRPs in the I/O Manager maintained queue in device object
	PKDEVICE_QUEUE_ENTRY QueueEntry;
	while( (QueueEntry=KeRemoveDeviceQueue(&fdo->DeviceQueue)) != NULL)
	{
		PIRP CancelIrp = CONTAINING_RECORD( QueueEntry, IRP, Tail.Overlay.DeviceQueueEntry);
		CancelIrp->Cancel = TRUE;
		CancelIrp->CancelIrql = OldIrql;
		CancelIrp->CancelRoutine = NULL;

		IoReleaseCancelSpinLock(OldIrql);
		DebugPrint("WdmIoDispatchCleanup: Cancelling %x %I",CancelIrp,CancelIrp);
		UnlockDevice(dx);
		CompleteIrp( CancelIrp, STATUS_CANCELLED);
		IoAcquireCancelSpinLock(&OldIrql);
	}
	IoReleaseCancelSpinLock(OldIrql);

	// Forcibly cancel any in-progress IRP
	if( dx->Timeout!=-1)
	{
		if( KeSynchronizeExecution( dx->InterruptObject, (PKSYNCHRONIZE_ROUTINE)CancelCurrentIrpSynch, dx))
		{
			if( fdo->CurrentIrp!=NULL)
			{
				DebugPrint("WdmIoDispatchCleanup: Cancelled in-progress IRP %x %I",fdo->CurrentIrp,fdo->CurrentIrp);
				WdmIoDpcForIsr( NULL, fdo, fdo->CurrentIrp, dx);
			}
		}
	}

	return CompleteIrp( Irp, STATUS_SUCCESS);
}

/////////////////////////////////////////////////////////////////////////////
