from engine import engine_factory


class Coach:

    def __init__(self):
        self.p0 = engine_factory()
        self.p1 = engine_factory()
        self.move_seq = ""

    def play(self):
        p1_turn = True
        move = self.p0.send_move(self.move_seq)
        i = 1
        print(str(i) + ". " + move)
        self.move_seq = move
        while move != 'draw' and move != 'loss':
            if p1_turn:
                move = self.p1.send_move(self.move_seq)
            else:
                move = self.p0.send_move(self.move_seq)
            i += 1
            print(str(i) + ". " + move)
            p1_turn = not p1_turn
            self.move_seq += move
        if move == 'draw':
            print('draw')
        elif p1_turn:
            print('black won')
        else:
            print('white won')


if __name__ == '__main__':
    c = Coach()
    c.play()




