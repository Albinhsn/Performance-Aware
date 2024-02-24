global parseString2


section .text

; key in rdi
; buffer in rsi

parseString2:
  ; mov curr to register
  ;   this is also start
  mov rdx, QWORD PTR [rsi + 8]
  mov rcx, QWORD PTR [rsi]

  ; set key->buffer with effective address of curr and buffer 
  lea rax, [rcx + rdx + 1]

  mov QWORD PTR [rdi], rax
  
  ; get the character at buffer[curr]
  ; check whether we loop or not
  cmp BYTE PTR [rax], 34
  je .foot
  

.loop:
  ; advance
  ; check '"'
  ; either fall to foot or back
  
.foot:
  ; set key->len 
  mov QWORD PTR [rdi + 8], 
  ; increase curr
  ; mov back curr to pointed address
  ret
  
