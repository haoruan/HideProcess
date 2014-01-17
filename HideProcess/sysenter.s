extern d_origKiFastCallEntry:QWORD
extern MyKiFastCallEntry:PROC

.data

.code
	JmpOrigEntry proc
		call MyKiFastCallEntry
		jmp d_origKiFastCallEntry
	JmpOrigEntry endp
end