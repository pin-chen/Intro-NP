with open('case.txt', 'r') as f:
    for str in f:
        code = str.split()[1]
        print("\tcase "+code+":{")
        print("\t    break;")
        print("\t}")