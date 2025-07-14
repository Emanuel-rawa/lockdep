rm -rf ./build
mkdir -p ./build
cd ./build
cmake ..
make

TEST_BINARIES=$(find . -maxdepth 1 -executable -regex '\.\/t[0-9][0-9].*' | sort)

echo "=================================="
echo "== LISTING BINARIES OF TEST SET =="
echo "=================================="
for test_binary in $TEST_BINARIES; do
    echo $test_binary
done

echo "====================================="
echo "== TESTING LD_PRELOADER INTERPOSER =="
echo "====================================="
for test_binary in $TEST_BINARIES; do

    echo "===================================="
    echo "== TESTING $test_binary =="

    LD_PRELOAD=./liblockdep_interpose.so $test_binary

    echo "===================================="
done
