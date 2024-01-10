pin -t tools/obj-intel64/track.so -- test_sample/mini_test/mini_test.exe test_sample/mini_test/cur_input > run1.log

pin -t tools/obj-intel64/track.so -- readelf -h test_sample/readelf/cur_input > run1.log

pin -t tools/obj-intel64/track.so -- infotocap test_sample/infotocap/cur_input > run1.log
