.code
	MySeAccessCheck proc

		push rax
		push rcx
		push rdx
		push r8
		push r9
		push r10
		push r11
		pushfq

		mov rax, 4321432143214321h
		call rax

		popfq
		pop r11
		pop r10
		pop r9
		pop r8
		pop rdx
		pop rcx
		pop rax

		sub     rsp,68h
		mov     rax,qword ptr [rsp+0B8h]
		mov     qword ptr [rsp+50h],rax

		push rax

		mov rax, 1234123412341234h
		jmp rax

	MySeAccessCheck endp
end