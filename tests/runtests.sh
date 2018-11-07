echo ""
echo "Unit Tests"

real_tests="build/tests/real_tests"

# Loop over compiled tests and run them.
for test_file in build/tests/*_tests
do
    if [ "$test_file" == "$real_tests" ]
    then
        continue
    fi
    # Only execute if result is a file.
    if test -f $test_file
    then
        # Log execution to file.
        if ./$test_file 2>&1 > /tmp/sky-test.log
        then
            rm -f /tmp/sky-test.log
        else
            # If error occurred then print off log.
            cat /tmp/sky-test.log
            exit 1
        fi
    fi
done

echo ""
