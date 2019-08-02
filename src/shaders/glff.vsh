;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; uniform constants
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; $MODE = "UniformRegister"
; $UNIFORM_VARS[0].name = "mvp_row0"
; $UNIFORM_VARS[0].type = "Float4"
; $UNIFORM_VARS[0].count = 1
; $UNIFORM_VARS[0].block = -1
; $UNIFORM_VARS[0].offset = 0
; $UNIFORM_VARS[1].name = "mvp_row1"
; $UNIFORM_VARS[1].type = "Float4"
; $UNIFORM_VARS[1].count = 1
; $UNIFORM_VARS[1].block = -1
; $UNIFORM_VARS[1].offset = 4
; $UNIFORM_VARS[2].name = "mvp_row2"
; $UNIFORM_VARS[2].type = "Float4"
; $UNIFORM_VARS[2].count = 1
; $UNIFORM_VARS[2].block = -1
; $UNIFORM_VARS[2].offset = 8
; $UNIFORM_VARS[3].name = "mvp_row3"
; $UNIFORM_VARS[3].type = "Float4"
; $UNIFORM_VARS[3].count = 1
; $UNIFORM_VARS[3].block = -1
; $UNIFORM_VARS[3].offset = 12

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; per-vertex attributes
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; $ATTRIB_VARS[0].name = "vert_pos"
; $ATTRIB_VARS[0].type = "Float4"
; $ATTRIB_VARS[0].location = 0

00 CALL_FS

01 ALU_PUSH_BEFORE :
    0    x: DOT4 R127.x, R1.x, C0.x
         y: DOT4 ____,   R1.y, C0.y
         z: DOT4 ____,   R1.z, C0.z
         w: DOT4 ____,   R1.w, C0.w

    1    x: DOT4 ____,   R1.x, C1.x
         y: DOT4 R127.y, R1.y, C1.y
         z: DOT4 ____,   R1.z, C1.z
         w: DOT4 ____,   R1.w, C1.w

    2    x: DOT4 ____,   R1.x, C2.x
         y: DOT4 ____,   R1.y, C2.y
         z: DOT4 R127.z, R1.z, C2.z
         w: DOT4 ____,   R1.w, C2.w

    3    x: DOT4 ____,   R1.x, C3.x
         y: DOT4 ____,   R1.y, C3.y
         z: DOT4 ____,   R1.z, C3.z
         w: DOT4 R127.w, R1.w, C3.w

02 EXP_DONE: POS0, R127.xyzw
END_OF_PROGRAM
