.code
	JumpTemplate proc

		push rax
		push rcx
		push rdx
		push r8
		push r9
		push r10
		push r11
		pushfq

		mov rcx, 12h
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

		push 12341234h
		mov dword ptr [rsp+4h], 56785678h
		ret

	JumpTemplate endp
end