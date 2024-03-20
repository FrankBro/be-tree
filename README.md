# BE-Tree

![Build Status]( https://travis-ci.org/FrankBro/be-tree.svg?branch=master "Build Status")

## Overview

The goal of the be-tree is to evaluate as fast as possible a large amount of boolean expressions. The biggest gain in speed comes from evaluating as little expressions as possible, based on creating a structure that splits the possible domain for each expression, memoization and short-circuiting of events not containing some attributes.

The following types are supported:

* Boolean
* Integer
* Float
* String
* Integer list
* String list

The expressions supported are as follow:

* Comparison: `<`, `<=`, `>` and `>=`. They work on integers and floats.
* Equality: `=` and `<>`. They work on integers, integer enums, floats and strings.
* Boolean: `and`, `or`, `not` and checking a boolean variable.
* Set: `in` and `not in`. Two possibilities:
    * value `in` variable. Here value is an integer or a string.
    * variable `in` list. Here list is a list of integers or strings.
* List: `one of`, `none of` and `all of`. They work on lists of integers or strings.
* Null/Empty: `is null`, `is not null` for variables and `is empty` for lists.
* Special: Very specific.

## Configuring

Before inserting expressions, we need to define the domains that will be used. A domain is a variable that will be present in an event. You must define:

* Name
* Type
* Allow undefined
* (optional) Min and max values

The different types of values:

| Type name     | C type        | Domain min    | Domain max    |
| ------------- | ------------- | ------------- | ------------- |
| Boolean       | `bool`        | `false`       | `true`        |
| Integer       | `int64_t`     | `INT64_MIN`   | `INT64_MAX`   |
| Float         | `double`      | `-DBL_MAX`    | `DBL_MAX`     |
| String        | `size_t`      | `0`           | `SIZE_MAX`    |
| Integer list  | `int64_t []`  | `INT64_MIN`   | `INT64_MAX`   |
| String list   | `size_t []`   | `0`           | `SIZE_MAX`    |
| Segment       | struct        | `X`           | `X`           |
| Frequency     | struct        | `X`           | `X`           |

Once all the domains have been configured, we can start inserting expressions. Few problems that can occur from bad configurations:

* If an event contains an attribute that does not exist, is the wrong type or does not contain an attribute that cannot be undefined.
* If an expression contains an attribute that does not exist, is the wrong type or would require the event to be out of bound.

As you can notice, the event itself can have a value out of bound, this might match not equal events.

## Inserting

Here are the steps when you insert an expression in the tree:

First we must call `betree_make_sub` for each expressions, which does the following.

* Parse: Make sure we can parse the expression. During that step we reorder the `and` and `or` expressions to put the simplest to evaluate first.
* Assign var ids: Every variables is assigned an id for quick fetching in the events later
* Validation:
    * Are all the variables in the expression present in the config
    * Make sure no integer enum is used in a comparision expression
* Assign constants: Replace all the constants in an expression to their value. This is only used for the frequency expression as of now.
* Assign str ids: Every string is assigned an id (unique to each variable) to compare on instead of doing string comparison
* Sort lists: Sort the integer and string lists in the expression, to speed up searching in it but also to make sure in the next step, the same list is registered as the same predicate.
* Fix floats with no fractions: Some expressions will use float variable to compare with what would be parsed as an integer. We will fix these to make sure the constants are floats.
* Change boundaries: We use the expression to adjust the boundaries of the variables in the configuration, trying to minimize the boundaries for better performance if possible
* Assign pred ids: We give a predicate id to every sub-expression that is present more than once, this will be used later by the memoize
* Create the sub, which will create the short-circuit bitmap explained later, and insert it in the tree

Once we've created a sub for every expressions we want to insert, we can finally call `betree_insert_sub` with all of them. 

From now on, we follow the be-tree logic, which goes as follow:

* Traverse the tree, using the scoring function to find which branch to use, and find the lowest possible node we can insert the expression into.
* If we found a node, insert. If the node is overflowing, begin the splitting process.
* If we haven't found a node, we will create a new cdir with the best attribute, using the scoring function, and insert. If the node is overflowing, begin the splitting process.

Finally, the splitting process:

* If we are not at the top of a cdir, try to split the domain further if possible. 
* If it's not possible, find the next highest ranked attribute, based on the score function
* Create a new pdir and try to move stuff in, continue splitting until satisfactory.
* If some nodes are overflowing but can't be split, we increase the capacity of the node.

## Matching

When receiving an event to match, we start by sorting the integer and string lists for faster matching then start traversing the tree. After that we traverse the tree and extract all the expressions we will need to evaluate, the evaluation will come once we're done traversing the tree. To traverse the tree, we look for every pnode and see if our event contains the attribute or if the attribute can be undefined. Variables that can be undefined can causes a lot of useless evaluations.

Once we have extracted all the expressions, we have a few techniques to try to evaluate as few of these as possible:

* Short circuit: When an expression is inserted, we figure out if some attributes being undefined will make the expression pass or fail for sure. If this is the case, we just return that result. If not, we have to evaluate.
* Memoize: For each sub-expression we found present more than once, we will check if it has been evaluated already. If it has, we will return the previous result found for that sub-expression.

At the end of this, we return a report with all the subscriptions id found to be true.

## Possible changes
* For cdir splitting, there was a bug in the original implementation. I went with searching both lchild AND rchild but we could go with middle + 1.

## Implementation
* For variables that allow undefined, keep in mind that matching will always need to go down pnode's with those variables. Therefore, we rank them worse in the scoring part because they will cause a bunch of useless evaluations.
* Short-circuiting is at the sub level, not at the individual node level. When it's at the node level, it's actually slower.

## TODO
* Float domain should allow a way to control the splitting of float values. Right now it splits like integers but that won't work well for values that have a small domain (eg -0.01 to 0.01). Use domain to find a good split
* betree_remove should remove useless preds from the memoize
* What if we wrote the lexers/parsers to have the set of possible attributes directly since we know them. While we never use the string attribute during runtime, it slows insertion.


## How to run test benchmark

### Install required dependency(below step is for installing dependency in ubuntu 20.04)
* Install valgrind

```bash
sudo apt install valgrind
```

* Install gsl-2.7 library library
```bash
wget https://mirrors.tripadvisor.com/gnu/gsl/gsl-2.7.tar.gz
tar -xf gsl-2.7.tar.gz
cd gsl-2.7/
./configure && make && make install
```

* Export gsl LD_LIBRARY_PATH
```bash
export LD_LIBRARY_PATH="/usr/local/lib/"
```

### Build test benchmark binary for execution
* Run `make build-test-benchmark` for building the test binary

### Input for test benchmark binary
* Test binary dependency on 4 files which are in the data folder refer README.md file inside the `data` folder for more information about each file and format

### How to run the test
* Once you have constructed all the test files inside the data folder run `./testbenchmark` to run a test. It would create a be-tree and query it. The default number of times it would query would be 10. If you want to modify then you need to pass a command line argument. For running 1-time command is `./testbenchmark 1`

