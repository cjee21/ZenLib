// Tests for ZenLib BitStream Fast and LE
// Using GoogleTest
// Modified from code generated with Google Gemini 3.5 Flash

#include <gtest/gtest.h>
#include "ZenLib/BitStream_Fast.h"
#include "ZenLib/BitStream_LE.h"
#include <vector>

using namespace ZenLib;

// -------------------------------------------------------------------
// Tests for BitStream_Fast
// -------------------------------------------------------------------

TEST(BitStreamFastTest, BasicInitialization) {
    int8u data[] = { 0x12, 0x34 };
    BitStream_Fast bs(data, sizeof(data));

    EXPECT_EQ(bs.Remain(), 16);             // Initialized in total bits
    EXPECT_EQ(bs.Offset_Get(), 0);
    EXPECT_EQ(bs.BitOffset_Get(), 0);       // 16 % 8 = 0
    EXPECT_FALSE(bs.BufferUnderRun);
}

TEST(BitStreamFastTest, GetB_ReadsMSBToLSB) {
    // 0x80 = 1000 0000 b
    int8u data[] = { 0x80 };
    BitStream_Fast bs(data, sizeof(data));

    // First bit is the MSB -> 1 (true)
    EXPECT_TRUE(bs.GetB());
    EXPECT_EQ(bs.Remain(), 7);
    EXPECT_EQ(bs.BitOffset_Get(), 7);

    // Second bit is 0 (false)
    EXPECT_FALSE(bs.GetB());
}

TEST(BitStreamFastTest, Get1_CrossesByteBoundary) {
    // 0x55 = 0101 0101 b, 0xAA = 1010 1010 b
    int8u data[] = { 0x55, 0xAA };
    BitStream_Fast bs(data, sizeof(data));

    // Read 6 bits from first byte: 010101 -> 0x15
    EXPECT_EQ(bs.Get1(6), 0x15);

    // Read next 4 bits (2 from first byte "01", 2 from second byte "10")
    // Merged layout pattern: 01 10 -> 0x06
    EXPECT_EQ(bs.Get1(4), 0x06);
    EXPECT_EQ(bs.Remain(), 6);
}

TEST(BitStreamFastTest, Get2_UnalignedThenMaxBits) {
    // We need enough bits for a partial read + a max 16-bit read.
    // Let's use 3 bytes (24 bits). 
    // Data: 0xBA, 0x98, 0x76 -> Binary: 10111010 10011000 01110110
    int8u data[] = { 0xBA, 0x98, 0x76 };
    BitStream_Fast bs(data, sizeof(data));

    // 1st Read: Slightly less than a full byte (e.g., 5 bits).
    // From 0xBA (10111 010), the first 5 MSB bits are 10111 -> 0x17
    EXPECT_EQ(bs.Get2(5), 0x17);
    EXPECT_EQ(bs.Remain(), 19);
    EXPECT_EQ(bs.BitOffset_Get(), 3); // 19 % 8 = 3 bits remaining in current byte group

    // 2nd Read: Maximum supported bits for Get2 (16 bits) across unaligned boundaries.
    // Remaining bits of 1st byte: 010 (3 bits)
    // All bits of 2nd byte:       10011000 (8 bits)
    // First 5 bits of 3rd byte:   01110 (5 bits from 0x76 [01110 110])
    // Combined target: 010 10011000 01110 -> 0x530E
    EXPECT_EQ(bs.Get2(16), 0x530E);
    EXPECT_EQ(bs.Remain(), 3);
    EXPECT_FALSE(bs.BufferUnderRun);
}

TEST(BitStreamFastTest, Get4_UnalignedThenMaxBits) {
    // 6 bytes = 48 bits total capacity
    // Values: 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
    // Binary: 10101010 10111011 11001100 11011101 11101110 11111111
    int8u data[] = { 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF };
    BitStream_Fast bs(data, sizeof(data));

    // 1st Read: 11 bits
    // 10101010 101 -> 0x555
    EXPECT_EQ(bs.Get4(11), 0x555);
    EXPECT_EQ(bs.Remain(), 37); // 48 - 11 = 37 bits left
    EXPECT_EQ(bs.BitOffset_Get(), 5); // 37 % 8 = 5 bits offset

    // 2nd Read: Max 32 bits across the unaligned boundary
    // Consumes: 
    // - 5 remaining bits of byte 1:  11011 (from 0xBB [10111 1011])
    // - 8 bits of byte 2:            11001100 (0xCC)
    // - 8 bits of byte 3:            11011101 (0xDD)
    // - 8 bits of byte 4:            11101110 (0xEE)
    // - 3 bits of byte 5:            111 (from 0xFF [111 11111])
    // Combined MSB-to-LSB: 11011 11001100 11011101 11101110 111 -> 0xDE66EF77
    EXPECT_EQ(bs.Get4(32), 0xDE66EF77);
    
    EXPECT_EQ(bs.Remain(), 5); // 37 - 32 = 5 bits left
    EXPECT_FALSE(bs.BufferUnderRun);
}

TEST(BitStreamFastTest, Get8_UnalignedThenMaxBits) {
    // Need enough bits for a partial read + a max 64-bit read.
    // Total space: 10 bytes (80 bits)
    int8u data[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22 };
    BitStream_Fast bs(data, sizeof(data));

    // 1st Read: Slightly less than 1 byte (e.g., 4 bits)
    // 0x12 (0001 0010) -> first 4 bits are 0001 -> 0x01
    EXPECT_EQ(bs.Get8(4), 0x01);
    EXPECT_EQ(bs.Remain(), 76);

    // 2nd Read: Maximum supported bits for Get8 (64 bits)
    // This grabs the remaining 4 bits of byte 0, bytes 1 through 7 completely, 
    // and the top 4 bits of byte 8.
    int64u result = bs.Get8(64);
    EXPECT_NE(result, 0);
    EXPECT_EQ(bs.Remain(), 12);
    EXPECT_FALSE(bs.BufferUnderRun); // Safely fits inside the 80-bit buffer window
}

TEST(BitStreamFastTest, Get1_UnsupportedNumberOfBits) {
    int8u data[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22 };
    BitStream_Fast bs(data, sizeof(data));

    // Test protection against invalid number of bits
    EXPECT_EQ(bs.Get1(9), 0);
    EXPECT_EQ(bs.Remain(), 80);
}

TEST(BitStreamFastTest, Get2_UnsupportedNumberOfBits) {
    int8u data[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22 };
    BitStream_Fast bs(data, sizeof(data));

    // Test protection against invalid number of bits
    EXPECT_EQ(bs.Get2(17), 0);
    EXPECT_EQ(bs.Remain(), 80);
}

TEST(BitStreamFastTest, Get4_UnsupportedNumberOfBits) {
    int8u data[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22 };
    BitStream_Fast bs(data, sizeof(data));

    // Test protection against invalid number of bits
    EXPECT_EQ(bs.Get4(33), 0);
    EXPECT_EQ(bs.Remain(), 80);
}

TEST(BitStreamFastTest, Get8_UnsupportedNumberOfBits) {
    int8u data[] = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0, 0x11, 0x22 };
    BitStream_Fast bs(data, sizeof(data));

    // Test protection against invalid number of bits
    EXPECT_EQ(bs.Get8(65), 0);
    EXPECT_EQ(bs.Remain(), 80);
}

TEST(BitStreamFastTest, PeekingDoesNotAdvanceStream) {
    int8u data[] = { 0xAB, 0xCD, 0xEF, 0x01, 0x23, 0x45 };
    BitStream_Fast bs(data, sizeof(data));

    // Peek the first 4 bits (0xA)
    EXPECT_EQ(bs.Peek1(4), 0x0A);
    // Ensure metrics are untouched
    EXPECT_EQ(bs.Remain(), 48);

    // Repeat for other Peeks
    EXPECT_EQ(bs.PeekB(), 0x1);
    EXPECT_EQ(bs.Remain(), 48);
    EXPECT_EQ(bs.Peek2(12), 0x0ABC);
    EXPECT_EQ(bs.Remain(), 48);
    EXPECT_EQ(bs.Peek4(20), 0x0ABCDE);
    EXPECT_EQ(bs.Remain(), 48);
    EXPECT_EQ(bs.Peek8(36), 0); // Not yet implemented
    EXPECT_EQ(bs.Remain(), 48);

    // Consume those 4 bits now
    EXPECT_EQ(bs.Get1(4), 0x0A);
    EXPECT_EQ(bs.Remain(), 44);

    // Peek next 4 bits from the dirty alignment boundary (0xB)
    EXPECT_EQ(bs.Peek1(4), 0x0B);
    EXPECT_EQ(bs.PeekB(), 0x1);
    EXPECT_EQ(bs.Peek2(12), 0x0BCD);
    EXPECT_EQ(bs.Peek4(20), 0x0BCDEF);
    EXPECT_EQ(bs.Peek8(36), 0); // Not yet implemented
    EXPECT_EQ(bs.Remain(), 44);
}

TEST(BitStreamFastTest, LargeSkipsAndByteAlign) {
    // 10 bytes of data
    int8u data[] = { 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x99 };
    BitStream_Fast bs(data, sizeof(data));

    // Skip 3 bits into byte 0
    bs.Skip(3);
    EXPECT_EQ(bs.BitOffset_Get(), 5); // 77 bits remain -> 77 % 8 = 5

    // Perform a large skip across 32-bit thresholds (skipping 63 bits total)
    // Remaining bits before: 77. 77 - 60 = 17 bits remaining
    bs.Skip(60);
    EXPECT_EQ(bs.Remain(), 17);

    // Align to the next byte boundary
    bs.Byte_Align(); // 17 % 8 = 1 bit to discard -> drops down to 16 bits
    EXPECT_EQ(bs.Remain(), 16);
    EXPECT_EQ(bs.BitOffset_Get(), 0);

    // Next read should target the start of byte index 8 (0x00) and index 9 (0x99)
    EXPECT_EQ(bs.Get2(16), 0x0099);
}

TEST(BitStreamFastTest, BeyondBufferUnderRunProtection) {
// Get1
{
    int8u data[] = { 0x11 };
    BitStream_Fast bs(data, sizeof(data)); // 8 bits total
    bs.Skip(4);
    int8u result = bs.Get1(5);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(bs.BufferUnderRun);
    EXPECT_EQ(bs.Remain(), 0);
}

// Get2
{
    int8u data[] = { 0x11 };
    BitStream_Fast bs(data, sizeof(data)); // 8 bits total
    int8u result = bs.Get2(12);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(bs.BufferUnderRun);
    EXPECT_EQ(bs.Remain(), 0);
}

// Get4
{
    int8u data[] = { 0x11 };
    BitStream_Fast bs(data, sizeof(data)); // 8 bits total
    int8u result = bs.Get4(12);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(bs.BufferUnderRun);
    EXPECT_EQ(bs.Remain(), 0);
}

// Get8
{
    int8u data[] = { 0x11 };
    BitStream_Fast bs(data, sizeof(data)); // 8 bits total
    int8u result = bs.Get8(12);
    EXPECT_EQ(result, 0);
    EXPECT_TRUE(bs.BufferUnderRun);
    EXPECT_EQ(bs.Remain(), 0);
}

// Skip
{
    int8u data[] = { 0x11 };
    BitStream_Fast bs(data, sizeof(data)); // 8 bits total
    bs.Skip(12);
    EXPECT_TRUE(bs.BufferUnderRun);
    EXPECT_EQ(bs.Remain(), 0);
}
}

TEST(BitStreamFastTest, ResizeAndClearUnderRun) {
    int8u data[] = { 0xFF };
    BitStream_Fast bs(data, sizeof(data));

    bs.Skip(16); // Force an explicit Underrun error
    EXPECT_TRUE(bs.BufferUnderRun);

    // Dynamically adjust internal sizes back up
    bs.Resize(8);
    EXPECT_FALSE(bs.BufferUnderRun);
    EXPECT_EQ(bs.Remain(), 8);
}

TEST(BitStreamFastTest, ByteAlignment) {
    int8u data[] = { 0xFF, 0xAA };
    BitStream_Fast bs(data, sizeof(data));

    bs.Get1(3); // Offset inside byte 0
    EXPECT_EQ(bs.BitOffset_Get(), 8-3);

    bs.Byte_Align();
    EXPECT_EQ(bs.BitOffset_Get(), 0);
    EXPECT_EQ(bs.Offset_Get(), 1); // Advanced to byte 1
}

// -------------------------------------------------------------------
// Tests for BitStream_LE
// -------------------------------------------------------------------

TEST(BitStreamLETest, BasicInitialization) {
    int8u data[] = { 0x01, 0x02 };
    BitStream_LE bs(data, sizeof(data));

    EXPECT_EQ(bs.Remain(), 16);
    EXPECT_EQ(bs.Offset_Get(), 0);
    EXPECT_EQ(bs.BitOffset_Get(), 0);
}

TEST(BitStreamLETest, ReadBitsLittleEndian) {
    // 0x0F = 0000 1111, 0xF0 = 1111 0000
    // Stream reads from LSB to MSB of each byte in Little Endian fashion
    int8u data[] = { 0x0F, 0xF0 }; 
    BitStream_LE bs(data, sizeof(data));

    // Read 4 bits: should be the lower 4 bits of 0x0F -> 0x0F & 0x0F = 0x0F (15)
    EXPECT_EQ(bs.Get(4), 0x0F);
    EXPECT_EQ(bs.BitOffset_Get(), 4);

    // Read next 4 bits: upper 4 bits of 0x0F -> 0x00
    EXPECT_EQ(bs.Get(4), 0x00);
    EXPECT_EQ(bs.BitOffset_Get(), 0);
    EXPECT_EQ(bs.Offset_Get(), 1); // Moved to next byte
}

TEST(BitStreamLETest, ReadOver8And16BitsAcrossBoundaries) {
    // 5 bytes of data for wide, unaligned reading
    // Byte 0: 0xFF (1111 1111)
    // Byte 1: 0x00 (0000 0000)
    // Byte 2: 0xAA (1010 1010)
    // Byte 3: 0x55 (0101 0101)
    // Byte 4: 0xFF (1111 1111)
    int8u data[] = { 0xFF, 0x00, 0xAA, 0x55, 0xFF };
    BitStream_LE bs(data, sizeof(data));

    // -------------------------------------------------------------------
    // 1. Setup Phase: Unalign the stream by reading 4 bits
    // -------------------------------------------------------------------
    EXPECT_EQ(bs.Get(4), 0x0F); // Lower 4 bits of 0xFF
    EXPECT_EQ(bs.BitOffset_Get(), 4);
    EXPECT_EQ(bs.Offset_Get(), 0);

    // -------------------------------------------------------------------
    // 2. Read 9 bits (Over 8-bit chunk boundary)
    // -------------------------------------------------------------------
    // Needs:
    // - 4 remaining bits from Byte 0 (0xFF): 1111 b
    // - 5 bits from Byte 1 (0x00):           00000 b
    //
    // Stitched together (Byte 1 bits occupy higher positions):
    // Result binary: [Byte 1: 00000] [Byte 0: 1111] -> 000001111 b = 0x00F
    EXPECT_EQ(bs.Get(9), 0x00F);
    
    // Total consumed: 4 + 9 = 13 bits. Offset should be Byte 1, bit index 5.
    EXPECT_EQ(bs.BitOffset_Get(), 5);
    EXPECT_EQ(bs.Offset_Get(), 1);

    // -------------------------------------------------------------------
    // 3. Read 17 bits (Over 16-bit wide chunk boundaries)
    // -------------------------------------------------------------------
    // Needs:
    // - 3 remaining bits from Byte 1 (0x00): 000 b
    // - 8 full bits from Byte 2 (0xAA):      10101010 b
    // - 6 bits from Byte 3 (0x55):           010101 b (lower 6 bits of 01010101)
    //
    // Stitched together (Highest bytes shifted left sequentially):
    // Position weights: [ Byte 3 (6 bits) ] [ Byte 2 (8 bits) ] [ Byte 1 (3 bits) ]
    // Binary:           [    010101     ] [   10101010    ] [       000      ]
    // Hex assembly:
    //   Byte 1: 000 b                     = 0x0
    //   Byte 2: 10101010 b << 3           = 0x550
    //   Byte 3: 010101 b   << 11          = 0xA800
    // Total Result: 0xA800 | 0x550 | 0x0   = 0xAD50
    EXPECT_EQ(bs.Get(17), 0xAD50);

    // Total consumed: 13 + 17 = 30 bits.
    // 30 bits / 8 = 3 bytes offset with 6 bits remaining.
    EXPECT_EQ(bs.BitOffset_Get(), 6);
    EXPECT_EQ(bs.Offset_Get(), 3);
    EXPECT_EQ(bs.Remain(), 10); // 40 total bits - 30 consumed = 10 left
}

TEST(BitStreamLETest, ByteAlignment) {
    int8u data[] = { 0xFF, 0xAA };
    BitStream_LE bs(data, sizeof(data));

    bs.Get(3); // Offset inside byte 0
    EXPECT_EQ(bs.BitOffset_Get(), 3);

    bs.Byte_Align();
    EXPECT_EQ(bs.BitOffset_Get(), 0);
    EXPECT_EQ(bs.Offset_Get(), 1); // Advanced to byte 1
}

// Focus: Tests for large skips over 32-bits
TEST(BitStreamLETest, LargeSkipsOver32Bits) {
    // 10 bytes of data (80 bits total)
    // Indexes:    0     1     2     3     4     5     6     7     8     9
    // Values:  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA
    std::vector<int8u> data = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0xAA};
    BitStream_LE bs(data.data(), data.size());

    // Total bits = 80. Skip 68 bits.
    // 68 bits / 8 = 8 bytes (64 bits) + 4 remaining bits.
    size_t skip_amount = 68;
    bs.Skip(skip_amount);

    EXPECT_EQ(bs.BitOffset_Get(), 4);
    EXPECT_EQ(bs.Offset_Get(), 8); // Points to byte index 8 (0x55)
    EXPECT_EQ(bs.Remain(), 12);   // 80 - 68 = 12 bits remaining

    // Byte index 8 is 0x55 (0101 0101). 
    // Bit offset is 4, so reading 4 bits should get the upper nibble: 0x05 (0101)
    EXPECT_EQ(bs.Get(4), 0x05); 
}

TEST(BitStreamLETest, SkipBeyondTotalStorage) {
    int8u data[] = { 0x11, 0x22 };
    BitStream_LE bs(data, sizeof(data));

    // Try to skip 100 bits (only 16 exist). Internal implementation triggers an overflow loop.
    bs.Skip(100);
    
    // According to your Attach(NULL, 0) logic inside overflow:
    EXPECT_EQ(bs.Remain(), 0);
}

TEST(BitStreamLETest, ReadOver32BitsSequentially) {
    // 8 bytes of data (64 bits total)
    // Little Endian layout: 0x1122334455667788 sequentially packed
    int8u data[] = { 0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 };
    BitStream_LE bs(data, sizeof(data));

    // Read first 32 bits (4 bytes)
    // LSB starts at data[0]. Val: 0x55667788
    int32u first_half = bs.Get(32);
    EXPECT_EQ(first_half, 0x55667788);

    // Read next 32 bits (4 bytes)
    // Val: 0x11223344
    int32u second_half = bs.Get(32);
    EXPECT_EQ(second_half, 0x11223344);

    EXPECT_EQ(bs.Remain(), 0);
}

TEST(BitStreamLETest, GetDirectlyOver32BitsFails) {
    int8u data[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    BitStream_LE bs(data, sizeof(data));

    // Your implementation explicitly guards against HowMany > 32 by returning 0
    int32u result = bs.Get(64);
    EXPECT_EQ(result, 0);
    
    // Check that stream state did not advance or corrupt due to the invalid call
    EXPECT_EQ(bs.Remain(), 64);
    EXPECT_EQ(bs.Offset_Get(), 0);
}

TEST(BitStreamLETest, ReadExactBufferBoundary) {
    int8u data[] = { 0xAA };
    BitStream_LE bs(data, sizeof(data));

    // Read exactly what is available
    EXPECT_EQ(bs.Get(8), 0xAA);
    EXPECT_EQ(bs.Remain(), 0);
}

TEST(BitStreamLETest, ReadBeyondBufferBitBoundary) {
    int8u data[] = { 0x55 }; // 8 bits total
    BitStream_LE bs(data, sizeof(data));

    // Read 6 bits successfully
    EXPECT_EQ(bs.Get(6), 0x15); // 0x55 is 01010101 -> lower 6 bits are 010101 (0x15)
    EXPECT_EQ(bs.Remain(), 2);

    // Attempting to read 4 more bits requires 10 bits total, exceeding our 8-bit buffer.
    // In your implementation, this condition `if(endbyte*8+(long)HowMany>storage*8)` triggers,
    // which detaches the buffer entirely via Attach(NULL, 0) and returns -1 (0xFFFFFFFF).
    int32u result = bs.Get(4);
    
    EXPECT_EQ(result, 0xFFFFFFFF); 
    EXPECT_EQ(bs.Remain(), 0); // Reset to 0 after buffer detached
}

TEST(BitStreamLETest, ReadBeyondBufferByteBoundary) {
    int8u data[] = { 0x11, 0x22 }; // 2 bytes available (16 bits)
    BitStream_LE bs(data, sizeof(data));

    // Requesting 24 bits right away is out of bounds
    int32u result = bs.Get(24);

    EXPECT_EQ(result, 0xFFFFFFFF);
    EXPECT_EQ(bs.Remain(), 0);
    EXPECT_EQ(bs.Offset_Get(), 0); // Detached state sets buffer pointer to NULL
}
