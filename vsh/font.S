.section .text
.global font_start
.global font_end

.align 4
font_start:
    .incbin "vsh/font.psfu"
font_end:
