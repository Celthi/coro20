target=fizz_coawait
target_path=build/$target
rm -rf $target_path
cmake --build build --config Debug --target $target
if [[ $? -ne 0 ]]; then
    echo "Build failed"
    exit 1
fi
$target_path
if [[ $? -ne 0 ]]; then
    echo "Run failed"
    exit 1
fi
