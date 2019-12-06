function gc_loop() {
	for (i = 0; i < 10; i++) {
		obj = []

		for (j = 0; j < 15; j++) {
			obj.push([10, 20])
		}
		Assert(obj.length == 15)

		obj1 = [3, 2]
		obj2 = [4, 2]
		obj3 = [5, 2]
		obj4 = [6, 2]
		obj5 = [7, 2]
		obj6 = [8, 2]
		obj7 = [9, 2]
		obj8 = [10, 2]
		for (j = 0; j < 4000; j++) {
			nested_obj = {a: 11, b: 12}
			if (i == loop_count - 1) {
				Assert(nested_obj.a == 1)
				Assert(nested_obj.b == 2)
			}
		}
		Assert(obj[0][0] == 10)
		Assert(obj[1][1] == 20)
	}
}

gc_loop()