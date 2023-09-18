echo ""
echo "Unit Tests"

real_tests="build/tests/real_tests"

log_file=/tmp/sky-test.log

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
        if ./$test_file 2>&1 > $log_file
        then
            echo 'no errors'
            rm -f $log_file
        else
            # If error occurred then print off log.
            echo "error in test; see $log_file"
            cat $log_file
            exit 1
        fi
    fi
done

echo ""
