function fib_n(curr, next, n) {
    if (n == 0) {
        return curr
    } else {
        return fib_n(next, curr+next, n-1)
    }
}

function fib(n) {
	result = fib_n(0, 1, n)
	Print("result: " + result)
    return result
}

Print(fib(40))
Assert(fib(40) == 102334155)