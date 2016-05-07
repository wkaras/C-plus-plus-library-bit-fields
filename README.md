# C-plus-plus-library-bit-fields
Flexible, portible, high-performance bit fields C++ library.

The bit fields are specified with a dummy structure where each byte in the structure corresponds to 1 bit in the actual bit field structure.  Bit fields can be laid out from least to most or most to least significant bit, under programmer control.  Can directly access bit fields in the register maps of devices that are not memory-mapped.  Can transparently swap endiance between the processor and the device.  With good optimization, performance is comparable to using base language bit fields.

Warning:  Compiling testbf.cpp took about 5 minutes on my computer with gcc.  But, I did buy the computer for $65 on ebay.
