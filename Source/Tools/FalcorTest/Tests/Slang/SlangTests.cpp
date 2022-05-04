/***************************************************************************
 # Copyright (c) 2015-21, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/

/** This file contains tests for Slang language features we depend on.

    The Slang project has its own tests, but it doesn't hurt to test the
    common usage inside Falcor to make sure things work for our purposes.
*/

#include "Testing/UnitTest.h"
#include "SlangShared.slang"

namespace Falcor
{
    namespace
    {
        void testEnum(GPUUnitTestContext& ctx, const std::string& shaderModel)
        {
            ctx.createProgram("Tests/Slang/SlangTests.cs.slang", "testEnum", Program::DefineList(), Shader::CompilerFlags::None, shaderModel);
            ctx.allocateStructuredBuffer("result", 12);
            ctx.runProgram(1, 1, 1);

            // Verify results.
            const uint32_t* result = ctx.mapBuffer<const uint32_t>("result");

            EXPECT_EQ(result[0], (uint32_t)Type1::A);
            EXPECT_EQ(result[1], (uint32_t)Type1::B);
            EXPECT_EQ(result[2], (uint32_t)Type1::C);
            EXPECT_EQ(result[3], (uint32_t)Type1::D);

            EXPECT_EQ(result[4], (uint32_t)Type2::A);
            EXPECT_EQ(result[5], (uint32_t)Type2::B);
            EXPECT_EQ(result[6], (uint32_t)Type2::C);
            EXPECT_EQ(result[7], (uint32_t)Type2::D);

            EXPECT_EQ(result[8], (uint32_t)Type3::A);
            EXPECT_EQ(result[9], (uint32_t)Type3::B);
            EXPECT_EQ(result[10], (uint32_t)Type3::C);
            EXPECT_EQ(result[11], (uint32_t)Type3::D);

            ctx.unmapBuffer("result");
        }

        uint64_t asuint64(double a) { return *reinterpret_cast<uint64_t*>(&a); }
    }

    /** Test that compares the enums generated by enum declarations in C++ vs Slang.

        The goal is to verify that the enums can be used interchangeable on the CPU/GPU
        without unexpected results. Note in most cases it'd be fine if the enums differ,
        but certain uses (flags we OR together etc.) must match.
    */
    GPU_TEST(SlangEnum)
    {
        testEnum(ctx, "");      // Use default shader model for the unit test system
        testEnum(ctx, "6_0");
        testEnum(ctx, "6_3");
    }

    /** Test fixed-width scalar type support including 16-bit types (shader model 6.2+).

        This test ensures that Slang supports HLSL 2018 and uses the '-enable-16bit-types'
        flag by default in shader model 6.2.
        https://github.com/Microsoft/DirectXShaderCompiler/wiki/16-Bit-Scalar-Types
    */
    GPU_TEST(SlangScalarTypes)
    {
        const uint32_t maxTests = 100;

        ctx.createProgram("Tests/Slang/SlangTests.cs.slang", "testScalarTypes", Program::DefineList(), Shader::CompilerFlags::None, "6_2");
        ctx.allocateStructuredBuffer("result", maxTests);
        ctx.runProgram(1, 1, 1);

        // Verify results.
        const uint32_t* result = ctx.mapBuffer<const uint32_t>("result");

        int i = 0;

        // float16_t
        EXPECT_NE(result[i], asuint(1 / 3.f));
        EXPECT_EQ(result[i], asuint(f16tof32(f32tof16(1 / 3.f)))); i++;

        // float32_t
        EXPECT_EQ(result[i], asuint(1 / 5.f)); i++;

        // float64_t
        EXPECT_EQ(result[i], (uint32_t)asuint64(1 / 7.0)); i++;
        EXPECT_EQ(result[i], (uint32_t)(asuint64(1 / 7.0) >> 32)); i++;

        // int16_t
        EXPECT_EQ(result[i], (uint32_t)30000); i++;
        EXPECT_EQ(result[i], (uint32_t)-3392); i++;

        // int32_t
        EXPECT_EQ(result[i], 291123); i++;
        EXPECT_EQ(result[i], (uint32_t)-2000000000); i++;

        // int64_t
        EXPECT_EQ(result[i], 0xaabbccdd); i++;
        EXPECT_EQ(result[i], 0x12345678); i++;

        // uint16_t
        EXPECT_EQ(result[i], 59123); i++;
        EXPECT_EQ(result[i], 65526); i++;

        // uint32_t
        EXPECT_EQ(result[i], 0xfedc1234); i++;
        EXPECT_EQ(result[i], (uint32_t)-129); i++;

        // uint64_t
        EXPECT_EQ(result[i], 0xaabbccdd); i++;
        EXPECT_EQ(result[i], 0x12345678); i++;

        ctx.unmapBuffer("result");
        assert(i < maxTests);
    }

    /** Test Slang default initializers for basic types and structs.
    */
    GPU_TEST(SlangDefaultInitializers)
    {
        const uint32_t maxTests = 100, usedTests = 43;
        std::vector<uint32_t> initData(maxTests, -1);

        auto test = [&](const std::string& shaderModel)
        {
            ctx.createProgram("Tests/Slang/SlangTests.cs.slang", "testDefaultInitializers", Program::DefineList(), Shader::CompilerFlags::None, shaderModel);
            ctx.allocateStructuredBuffer("result", maxTests, initData.data(), initData.size() * sizeof(initData[0]));
            ctx.runProgram(1, 1, 1);

            // Verify results.
            const uint32_t* result = ctx.mapBuffer<const uint32_t>("result");
            for (uint32_t i = 0; i < maxTests; i++)
            {
                uint32_t expected = i < usedTests ? 0 : -1;
                if (i == 42) expected = (uint32_t)Type3::C;

                EXPECT_EQ(result[i], expected) << "i = " << i << " (sm" << shaderModel << ")";
            }
            ctx.unmapBuffer("result");
        };

        // Test the default shader model, followed by specific models.
        test("");
        test("6_0");
        test("6_1");
        test("6_2");
        test("6_3");
        test("6_5");
#if _ENABLE_D3D12_AGILITY_SDK
        test("6_6");
#endif
    }

    GPU_TEST(SlangHashedStrings)
    {
        ctx.createProgram("Tests/Slang/SlangTests.cs.slang", "testHashedStrings");
        ctx.allocateStructuredBuffer("result", 4);
        ctx.runProgram(1, 1, 1);

        auto hashedStrings = ctx.getProgram()->getReflector()->getHashedStrings();
        EXPECT_EQ(hashedStrings.size(), 4);
        EXPECT_EQ(hashedStrings[0].string, "Test String 0");
        EXPECT_EQ(hashedStrings[1].string, "Test String 1");
        EXPECT_EQ(hashedStrings[2].string, "Test String 2");
        EXPECT_EQ(hashedStrings[3].string, "Test String 3");

        const uint32_t* result = ctx.mapBuffer<const uint32_t>("result");

        for (size_t i = 0; i < 4; ++i)
        {
            EXPECT_EQ(result[i], hashedStrings[i].hash);
        }

        ctx.unmapBuffer("result");
    }
}
