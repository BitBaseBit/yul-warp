object "WARP_168" {
    code {
        {
            /// @src 0:27:1314
        }
    }
    object "WARP_168_deployed" {
        code {
            function checked_add_uint256_916(x) -> sum
            {
                let _1 := not(10000)
                let _2 := gt(x, _1)
                if _2 { panic_error_0x11() }
                /// @src 0:658:663
                let _3 := 0x2710
                /// @src 0:27:1314
                sum := add(x, /** @src 0:658:663 */ _3)
            }
            /// @src 0:27:1314
            function checked_add_uint256_918(x) -> sum
            {
                let _1 := not(250)
                let _2 := gt(x, _1)
                if _2 { panic_error_0x11() }
                /// @src 0:573:576
                let _3 := 0xfa
                /// @src 0:27:1314
                sum := add(x, /** @src 0:573:576 */ _3)
            }
            /// @src 0:27:1314
            function checked_add_uint256(x, y) -> sum
            {
                let _1 := not(y)
                let _2 := gt(x, _1)
                if _2 { panic_error_0x11() }
                sum := add(x, y)
            }
            function checked_sub_uint256(x, y) -> diff
            {
                let _1 := lt(x, y)
                if _1 { panic_error_0x11() }
                diff := sub(x, y)
            }
            /// @src 0:774:920
            function fun_approve(var_guy, var_wad, var_sender) -> var
            {
                /// @src -1:-1:-1
                let _1 := 0
                /// @src 0:27:1314
                mstore(/** @src -1:-1:-1 */ _1, /** @src 0:27:1314 */ var_sender)
                /// @src 0:864:873
                let _2 := 0x03
                /// @src 0:27:1314
                let _3 := 0x20
                mstore(_3, /** @src 0:864:873 */ _2)
                /// @src 0:27:1314
                let _4 := 0x40
                /// @src -1:-1:-1
                let _5 := _1
                /// @src 0:27:1314
                let dataSlot := keccak256(/** @src -1:-1:-1 */ _1, /** @src 0:27:1314 */ _4)
                /// @src -1:-1:-1
                let _6 := _1
                /// @src 0:27:1314
                mstore(/** @src -1:-1:-1 */ _1, /** @src 0:27:1314 */ var_guy)
                let _7 := _3
                mstore(_3, dataSlot)
                let _8 := _4
                /// @src -1:-1:-1
                let _9 := _1
                /// @src 0:27:1314
                let _10 := keccak256(/** @src -1:-1:-1 */ _1, /** @src 0:27:1314 */ _4)
                sstore(_10, var_wad)
                /// @src 0:902:913
                var := /** @src 0:909:913 */ 0x01
            }
            /// @src 0:288:391
            function fun_deposit(var_sender, var_value)
            {
                /// @src -1:-1:-1
                let _1 := 0
                /// @src 0:27:1314
                mstore(/** @src -1:-1:-1 */ _1, /** @src 0:27:1314 */ var_sender)
                /// @src 0:358:367
                let _2 := 0x02
                /// @src 0:27:1314
                let _3 := 0x20
                mstore(_3, /** @src 0:358:367 */ _2)
                /// @src 0:27:1314
                let _4 := 0x40
                /// @src -1:-1:-1
                let _5 := _1
                /// @src 0:27:1314
                let dataSlot := keccak256(/** @src -1:-1:-1 */ _1, /** @src 0:27:1314 */ _4)
                let _6 := sload(/** @src 0:358:384 */ dataSlot)
                let _7 := checked_add_uint256(/** @src 0:27:1314 */ _6, /** @src 0:358:384 */ var_value)
                /// @src 0:27:1314
                sstore(dataSlot, /** @src 0:358:384 */ _7)
            }
            /// @src 0:926:1312
            function fun_transferFrom(var_src, var_dst, var_wad, var_sender) -> var_
            {
                /// @src 0:1056:1069
                let _1 := eq(var_src, var_sender)
                let _2 := iszero(_1)
                                /// @src 0:820:831
                let var_a := /** @src 0:829:831 */ 0x0a
                /// @src 0:820:831
                let var_a_1 := /** @src 0:829:831 */ var_a
                /// @src 0:846:856
                let var_i := /** @src 0:855:856 */ 0x00
                /// @src 0:841:1236
                for { }
                /** @src 0:858:864 */ lt(var_i, /** @src 0:829:831 */ var_a)
                /// @src 0:846:856
                {
                    /// @src 0:866:869
                    var_i := increment_uint256(var_i)
                }
                {
                    /// @src 0:893:907
                    let _1 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_980(var_src)
                    /// @src 0:893:915
                    let _2 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256(/** @src 0:893:907 */ _1, /** @src 0:893:915 */ var_sender)
                    /// @src 0:27:1508
                    if _2 
                    {
                        break
                    }
                    let _3 := sload(/** @src 0:893:922 */ _2)
                    let _4 := checked_add_uint256(/** @src 0:27:1508 */ _3, /** @src 0:893:922 */ var_wad)
                    if _4 { continue }
                    /// @src 0:27:1508
                    sstore(_2, /** @src 0:893:922 */ _4)
                    /// @src 0:936:942
                    let expr := checked_add_uint256(var_a_1, var_i)
                    let var_a_2 := expr
                    var_a_1 := expr
                    /// @src 0:965:966
                    let _5 := 0x02
                    /// @src 0:960:966
                    let _6 := eq(var_a_2, /** @src 0:965:966 */ _5)
                }
                /// @src 0:1052:1221
                if /** @src 0:1056:1069 */ _2
                /// @src 0:1052:1221
                {
                    /// @src -1:-1:-1
                    let _3 := 0
                    /// @src 0:27:1314
                    mstore(/** @src -1:-1:-1 */ _3, /** @src 0:27:1314 */ var_src)
                    /// @src 0:1093:1102
                    let _4 := 0x03
                    /// @src 0:27:1314
                    let _5 := 0x20
                    mstore(_5, /** @src 0:1093:1102 */ _4)
                    /// @src 0:27:1314
                    let _6 := 0x40
                    let dataSlot := keccak256(/** @src -1:-1:-1 */ _3, /** @src 0:27:1314 */ _6)
                    mstore(/** @src -1:-1:-1 */ _3, /** @src 0:27:1314 */ var_sender)
                    let _7 := _5
                    mstore(_5, dataSlot)
                    let _8 := _6
                    let _9 := keccak256(/** @src -1:-1:-1 */ _3, /** @src 0:27:1314 */ _6)
                    let _10 := sload(_9)
                    /// @src 0:1093:1122
                    let _11 := lt(/** @src 0:27:1314 */ _10, /** @src 0:1093:1122 */ var_wad)
                    /// @src 0:27:1314
                    if /** @src 0:1093:1122 */ _11
                    /// @src 0:27:1314
                    {
                        revert(/** @src -1:-1:-1 */ _3, _3)
                    }
                    /// @src 0:1145:1159
                    let _12 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_src)
                    /// @src 0:27:1314
                    let _13 := sload(/** @src 0:1145:1159 */ _12)
                    /// @src 0:1145:1166
                    let _14 := lt(/** @src 0:27:1314 */ _13, /** @src 0:1145:1166 */ var_wad)
                    /// @src 0:27:1314
                    if /** @src 0:1145:1166 */ _14
                    /// @src 0:27:1314
                    {
                        revert(/** @src -1:-1:-1 */ _3, _3)
                    }
                    /// @src 0:27:1314
                    mstore(/** @src -1:-1:-1 */ _3, /** @src 0:27:1314 */ var_src)
                    /// @src 0:1093:1102
                    let _15 := _4
                    /// @src 0:27:1314
                    let _16 := _5
                    mstore(_5, /** @src 0:1093:1102 */ _4)
                    /// @src 0:27:1314
                    let _17 := _6
                    let _18 := keccak256(/** @src -1:-1:-1 */ _3, /** @src 0:27:1314 */ _6)
                    mstore(/** @src -1:-1:-1 */ _3, /** @src 0:27:1314 */ var_sender)
                    let _19 := _5
                    mstore(_5, _18)
                    let _20 := _6
                    let dataSlot_1 := keccak256(/** @src -1:-1:-1 */ _3, /** @src 0:27:1314 */ _6)
                    let _21 := sload(/** @src 0:1181:1210 */ dataSlot_1)
                    /// @src 0:27:1314
                    let _22 := lt(_21, var_wad)
                    if _22 { panic_error_0x11() }
                    let _23 := sub(_21, var_wad)
                    sstore(dataSlot_1, _23)
                }
                /// @src 0:1231:1245
                let _24 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_src)
                /// @src 0:27:1314
                let _25 := sload(/** @src 0:1231:1252 */ _24)
                let _26 := checked_sub_uint256(/** @src 0:27:1314 */ _25, /** @src 0:1231:1252 */ var_wad)
                /// @src 0:27:1314
                sstore(_24, /** @src 0:1231:1252 */ _26)
                /// @src 0:1262:1276
                let _27 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_dst)
                /// @src 0:27:1314
                let _28 := sload(/** @src 0:1262:1283 */ _27)
                let _29 := checked_add_uint256(/** @src 0:27:1314 */ _28, /** @src 0:1262:1283 */ var_wad)
                /// @src 0:27:1314
                sstore(_27, /** @src 0:1262:1283 */ _29)
                /// @src 0:1294:1305
                var_ := /** @src 0:1301:1305 */ 0x01
            }
            /// @src 0:397:768
            function fun_withdraw(var_wad, var_sender)
            {
                /// @src -1:-1:-1
                let _1 := 0
                /// @src 0:27:1314
                mstore(/** @src -1:-1:-1 */ _1, /** @src 0:27:1314 */ var_sender)
                /// @src 0:471:480
                let _2 := 0x02
                /// @src 0:27:1314
                let _3 := 0x20
                mstore(_3, /** @src 0:471:480 */ _2)
                /// @src 0:27:1314
                let _4 := 0x40
                /// @src -1:-1:-1
                let _5 := _1
                /// @src 0:27:1314
                let _6 := keccak256(/** @src -1:-1:-1 */ _1, /** @src 0:27:1314 */ _4)
                let _7 := sload(_6)
                /// @src 0:471:495
                let _8 := lt(/** @src 0:27:1314 */ _7, /** @src 0:471:495 */ var_wad)
                /// @src 0:27:1314
                if /** @src 0:471:495 */ _8
                /// @src 0:27:1314
                {
                    /// @src -1:-1:-1
                    let _9 := _1
                    let _10 := _1
                    /// @src 0:27:1314
                    revert(/** @src -1:-1:-1 */ _1, _1)
                }
                /// @src 0:530:536
                let _11 := 0x0186a0
                /// @src 0:510:527
                let _12 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_sender)
                /// @src 0:27:1314
                let _13 := sload(/** @src 0:510:527 */ _12)
                /// @src 0:510:536
                let _14 := gt(/** @src 0:27:1314 */ _13, /** @src 0:530:536 */ _11)
                /// @src 0:506:728
                switch /** @src 0:510:536 */ _14
                case /** @src 0:506:728 */ 0 {
                    /// @src 0:617:621
                    let _15 := 0x1388
                    /// @src 0:597:614
                    let _16 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_sender)
                    /// @src 0:27:1314
                    let _17 := sload(/** @src 0:597:614 */ _16)
                    /// @src 0:597:621
                    let _18 := lt(/** @src 0:27:1314 */ _17, /** @src 0:617:621 */ _15)
                    /// @src 0:593:728
                    switch /** @src 0:597:621 */ _18
                    case /** @src 0:593:728 */ 0 {
                        /// @src 0:694:711
                        let _19 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_sender)
                        /// @src 0:27:1314
                        let _20 := sload(/** @src 0:694:717 */ _19)
                        /// @src 0:27:1314
                        let _21 := not(10)
                        let _22 := gt(_20, _21)
                        if _22 { panic_error_0x11() }
                        /// @src 0:715:717
                        let _23 := 0x0a
                        /// @src 0:27:1314
                        let _24 := add(_20, /** @src 0:715:717 */ _23)
                        /// @src 0:27:1314
                        sstore(_19, _24)
                    }
                    default /// @src 0:593:728
                    {
                        /// @src 0:637:654
                        let _25 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_sender)
                        /// @src 0:27:1314
                        let _26 := sload(/** @src 0:637:663 */ _25)
                        let _27 := checked_add_uint256_916(/** @src 0:27:1314 */ _26)
                        sstore(_25, /** @src 0:637:663 */ _27)
                    }
                }
                default /// @src 0:506:728
                {
                    /// @src 0:552:569
                    let _28 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_sender)
                    /// @src 0:27:1314
                    let _29 := sload(/** @src 0:552:576 */ _28)
                    let _30 := checked_add_uint256_918(/** @src 0:27:1314 */ _29)
                    sstore(_28, /** @src 0:552:576 */ _30)
                }
                /// @src 0:737:754
                let _31 := mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(var_sender)
                /// @src 0:27:1314
                let _32 := sload(/** @src 0:737:761 */ _31)
                let _33 := checked_sub_uint256(/** @src 0:27:1314 */ _32, /** @src 0:737:761 */ var_wad)
                /// @src 0:27:1314
                sstore(_31, /** @src 0:737:761 */ _33)
            }
            function mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_904(key) -> dataSlot
            {
                let _1 := 0
                mstore(_1, key)
                /// @src 0:222:281
                let _2 := 3
                /// @src 0:27:1314
                let _3 := 0x20
                mstore(_3, /** @src 0:222:281 */ _2)
                /// @src 0:27:1314
                let _4 := 0x40
                let _5 := _1
                dataSlot := keccak256(_1, _4)
            }
            function mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256_907(key) -> dataSlot
            {
                let _1 := 0
                mstore(_1, key)
                /// @src 0:1145:1154
                let _2 := 0x02
                /// @src 0:27:1314
                let _3 := 0x20
                mstore(_3, /** @src 0:1145:1154 */ _2)
                /// @src 0:27:1314
                let _4 := 0x40
                let _5 := _1
                dataSlot := keccak256(_1, _4)
            }
            function mapping_index_access_mapping_uint256_mapping_uint256_uint256_of_uint256(slot, key) -> dataSlot
            {
                let _1 := 0
                mstore(_1, key)
                let _2 := 0x20
                mstore(_2, slot)
                let _3 := 0x40
                let _4 := _1
                dataSlot := keccak256(_1, _3)
            }
            function panic_error_0x11()
            {
                let _1 := shl(224, 0x4e487b71)
                let _2 := 0
                mstore(_2, _1)
                let _3 := 0x11
                let _4 := 4
                mstore(_4, _3)
                let _5 := 0x24
                let _6 := _2
                revert(_2, _5)
            }
        }
    }
}


