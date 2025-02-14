.globl start

/* multiboot header */
MULTIBOOT_HEADER_MAGIC = 0x1BADB002
MULTIBOOT_HEADER_FLAGS = 0x00000000
MULTIBOOT_HEADER_CHECKSUM = -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.section ".multiboot"
.align 4
.long MULTIBOOT_HEADER_MAGIC
.long MULTIBOOT_HEADER_FLAGS
.long MULTIBOOT_HEADER_CHECKSUM

.text
.code32
start:
	movl $0xB8000, %edi
	movl $0x2f452f48, (%edi)
	addl $4, %edi
	movl $0x2f4C2f4C, (%edi)
	addl $4, %edi
	movl $0x2f2c2f4f, (%edi)
	addl $4, %edi
	movl $0x2f422f50, (%edi)
	addl $4, %edi
	movl $0x2f312f32, (%edi)
	addl $4, %edi
	movl $0x2f322f30, (%edi)
	addl $4, %edi
	movl $0x2f362f30, (%edi)
	addl $4, %edi
	movl $0x2f312f35, (%edi)
	addl $4, %edi
	movl $0x2f552f57, (%edi)
	addl $4, %edi
	movl $0x2f552f59, (%edi)
	addl $4, %edi
	movl $0x2f492f58, (%edi)
	addl $4, %edi
	movl $0x2f472f4E, (%edi)
    	hlt

