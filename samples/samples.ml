# s 1
# an assignment statement, nothing is printed
x <- 2.3

# s 2
# an assignment statement, 2.500000 is printed
x <- 2.5
print x

# s 3
# 3.500000 is printed
print 3.5

# s 4
# 24 is printed
x <- 8
y <- 3
print x * y

# s 5
# 18 is printed
#
function printsum a b
	print a + b
#
printsum (12, 6)

# s 6
# 72 is printed
#
function multiply a b
	return a * b
#
print multiply(12, 6)

# s 7
# 50 is printed
#
function multiply a b
	x <- a * b
	return x
#
print multiply(10, 5)

# s 8
# 9 is printed
#
one <- 1
#
function increment value
	return value + one
#
print increment(3) + increment(4)