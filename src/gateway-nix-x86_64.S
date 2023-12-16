#
#  -----------------------------------
# |         JNIHook - by rdbo         |
# |      Java VM Hooking Library      |
#  -----------------------------------
#

#
# Copyright (C) 2023    Rdbo
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License version 3
# as published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
#

.text
.global jnihook_gateway
.extern JNIHook_CallHandler

jnihook_gateway:
        # Store pointer to arguments (RSP + wordSize to skip the return address)
        lea 8(%rsp), %rax

        # Back up registers
        push %rbx
        push %rcx
        push %rdx
        push %rsi
        push %rdi
        push %r8
        push %r9
        push %r10
        push %r11
        push %r12
        push %r13
        push %r14
        push %r15

        # Setup and call the CallHandler
        mov %rbx, %rdi
        mov %rax, %rsi
        mov %r15, %rdx
        call JNIHook_CallHandler

        # Restore the registers
        pop %r15
        pop %r14
        pop %r13
        pop %r12
        pop %r11
        pop %r10
        pop %r9
        pop %r8
        pop %rdi
        pop %rsi
        pop %rdx
        pop %rcx
        pop %rbx

        # Return to the interpreter
        # NOTE: the return value from the callback
        # is stored in %rax
        ret
