import cachepack

mc = cachepack.Client(["127.0.0.1:11211"])
mc.set("23","uha")
mc.set("sa",12)
mc.set("sa",[212,34,{"hoge":12}])
