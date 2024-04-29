with open("out.bin", "rb") as f:
    l = []
    for _ in range(2):
        count = f.read(1)[0]

        x = []
        for _ in range(count):
            x.append(f.read(6))
        l.append(x)

    fs_index = f.read(1)[0]
    ts_index = f.read(1)[0]
    step_count = f.read(1)[0]

    q = []
    for s in f.read(step_count):
        q.append(s)

    print(
        "\n".join(
            [
                k
                for block in ["{0:0>8b}".format(k) for k in l[0][0]]
                for k in [block[:4], block[4:]]
            ]
        )
    )
    print("")
    print(
        "\n".join(
            [
                k
                for block in ["{0:0>8b}".format(k) for k in l[1][0]]
                for k in [block[:4], block[4:]]
            ]
        )
    )

    for k in q:
        print((k >> 3), (k >> 1) & 3, k & 1)
