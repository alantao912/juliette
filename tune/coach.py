import os

from engine import source_dir
from engine import tune_dir


class Coach:

    def __init__(self):
        self.p0 = None
        self.p1 = None
        self.prefix = 'weights'
        self.suffix = '.h'
        self.max_version = self.calc_max_version()
        self.move_seq = ""

    def calc_max_version(self) -> int:
        max_version: int = -1
        os.chdir(tune_dir + '\\data')
        files = os.listdir()
        for file in files:
            version = int(file.replace(self.prefix, '', 1).replace(self.suffix, '', 1))
            max_version = max(max_version, version)
        return max_version

    def clean_data(self) -> None:
        os.chdir(tune_dir + '\\data')
        files = os.listdir()
        for file in files:
            os.remove(file)
        self.max_version = 0
        os.system('copy ' + '..\\..\\src\\' + self.prefix + self.suffix + ' ' + self.prefix + str(self.max_version) +
                  self.suffix)

    def update_src(self) -> None:
        os.chdir(tune_dir + '\\data')
        files = os.listdir()
        for file in files:
            version = int(file.replace(self.prefix, '', 1).replace(self.suffix, '', 1))
            if version == self.max_version:
                os.system('del ..\\..\\src\\' + self.prefix + self.suffix + ' ')
                os.chdir(source_dir)
                os.system('copy ..\\tune\\data\\' + self.prefix + str(
                    version) + self.suffix + ' ' + self.prefix + self.suffix)
                break

    def train(self):

        print(self.max_version)

    """
        :description: Simulates a game between engines p0, and p1.
        :return: a string, in list format, containing moves played and outcome.
        
        Outcome Values: -1 -> Black Wins. 0 -> Draw. 1 -> White Wins
    """

    def __play(self) -> (str, int):
        p1_turn = True
        game_log = ""
        move = self.p0.send_move(self.move_seq)
        i = 1
        game_log += str(i) + ". " + move + '\n'
        self.move_seq = move
        while move != 'draw ' and move != 'loss ':
            if p1_turn:
                move = self.p1.send_move(self.move_seq)
            else:
                move = self.p0.send_move(self.move_seq)
            i += 1
            game_log += str(i) + ". " + move + '\n'
            p1_turn = not p1_turn
            self.move_seq += move
        outcome: int
        if move == 'draw ':
            game_log += 'draw\n'
            outcome = 0
        elif p1_turn:
            game_log += 'black won\n'
            outcome = -1
        else:
            game_log += 'white won\n'
            outcome = 1
        self.move_seq = None
        return game_log, outcome


if __name__ == '__main__':
    c = Coach()
    c.clean_data()
