PostBuildSteps: $(TARGET)
!if "$(DDKBUILDENV)"=="free"
	rebase -B 0x10000 -X . $(TARGET)
!endif
	copy $(TARGET) $(WINDIR)\system32\drivers
#	copy $(TARGET) $(WINDIR)\system32\drivers\hidusb.sys
