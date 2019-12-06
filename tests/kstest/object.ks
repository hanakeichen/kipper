obj = {}
Assert(obj["key1"] == undefined)
obj["key1"] = 2
Assert(obj["key1"] == 2)
Print(obj)