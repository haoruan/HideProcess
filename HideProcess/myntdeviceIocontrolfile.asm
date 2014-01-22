
.code
	MyNtDeviceIoControlFile proc
		push rax
		mov rax, 4321432143214321h
		call rax
		pop rax

		sub rsp,68h
		mov eax,dword ptr [rsp+0B8h]
		mov byte ptr [rsp+50h],1

		push rax
		mov rax, 1234123412341234h
		jmp rax
	MyNtDeviceIoControlFile endp
end