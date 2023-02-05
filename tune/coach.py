import time

from engine import engine_factory
from engine import source_dir
from format import *


def load_game(index: int) -> str:
    os.chdir(tune_dir)
    tests = open('test_suite', 'r')
    for i in range(index):
        tests.readline()
    return tests.readline()


def parse_game(game_str: str) -> (str, str):
    o_brace: int = game_str.index('[') + 1
    c_brace: int = game_str.index('] ')
    return game_str[o_brace:c_brace:1] + ' ', game_str[c_brace + len('] '):]


def init_game_log(opening: str) -> str:
    move_num: int = 1
    log = ""

    for move in opening.split():
        log += str(move_num) + '. ' + move + '\n'
        move_num += 1
    return log


def save_game_log(game_log: str, opening: str) -> None:
    file_buff = "Opening: " + opening + '\n'
    file_buff += game_log.replace('\n', '\n\t')
    title = "Game - " + str(time.time())
    write_to('games\\' + title, file_buff)


class Coach:

    def __init__(self):
        self.p0 = None
        self.p1 = None
        self.prefix = 'weights'
        self.suffix = '.h'
        self.iteration_map = dict()
        for i in range(0, 13 * 64, 1):
            self.iteration_map[i] = 0
        self.exp = '_exp'
        self.max_version = self.calc_max_version()
        self.move_seq = ""
        self.dx = 2

    def calc_max_version(self) -> int:
        max_version: int = -1
        os.chdir(tune_dir + '\\data')
        files = os.listdir()
        for file in files:
            if '.exe' in file:
                continue
            version = int(file.replace(self.prefix, '', 1).replace(self.suffix, '', 1))
            max_version = max(max_version, version)
        return max_version

    """
    :Description: Clears previous weights[%d].h files. Re-initializes data/ to weights0.h copied from src/weights.h
    """

    def clean_data(self) -> None:
        os.chdir(tune_dir + '\\data')
        files = os.listdir()
        for file in files:
            os.remove(file)
        self.max_version = 0
        os.system('copy ' + '..\\..\\src\\' + self.prefix + self.suffix + ' ' + self.prefix + str(self.max_version) +
                  self.suffix)

    """
    :Description: Finds the file with the highest version number, and replaces the existing weights.h file
    """

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

    def train_psqt(self):
        self.update_src()
        self.p0 = engine_factory()
        template: str = generate_psqt_template('data\\' + self.prefix + str(self.max_version) + self.suffix)
        for i in range(0, 13 * 64, 1):
            offset = self.generate_vector()
            magnitude: int = self.dx
            limit: int = 60
            elo = 0
            while elo == 0 and abs(magnitude) < limit:
                experimental_values: str = fill_displacements(template, '%d', offset)
                self.write_src(experimental_values)
                elo = self.test()
                self.amplify_vector(offset)
                magnitude += self.dx
                pass

        print(self.max_version)

    def write_src(self, content: str) -> None:
        os.chdir(source_dir)
        weights = open(self.prefix + self.suffix, 'w')
        weights.write(content)

    """
        :Description: Generates a test 
        
        :return: Change in elo from 1000 of p1 against p0. Assuming both p1 and p0 are 1000 rated to start off.
    """

    def test(self) -> int:
        self.p1 = engine_factory()
        test_num: int = 0

        # p0 moves first, is white.
        p0_wins: int = 0
        p1_wins: int = 0
        draws: int = 0
        game_str: str = load_game(test_num)
        while len(game_str) != 0:
            self.move_seq, opening_name = parse_game(game_str)
            print('Opening: ' + opening_name)
            game_log, outcome = self.__play()
            save_game_log(game_log, opening_name)
            if outcome == 1:
                p0_wins += 1
            elif outcome == -1:
                p1_wins += 1
            else:
                draws += 1
            print(opening_name + " " + str(outcome))
            self.move_seq = parse_game(game_str)[0]

            # Switch sides
            self.p0, self.p1 = self.p1, self.p0
            game_log, outcome = self.__play()
            save_game_log(game_log, opening_name)
            if outcome == 1:
                p1_wins += 1
            elif outcome == -1:
                p0_wins += 1
            else:
                draws += 1
            print(opening_name + " " + str(outcome))
            test_num += 1
            game_str = load_game(test_num)

        tot_games: int = p0_wins + p1_wins + draws
        return int(400 * (p1_wins - p0_wins) / float(tot_games))

    def generate_vector(self) -> List[int]:
        v: List[int] = []
        for i in range(0, 13 * 64, 1):
            v.append(0)
        for i in range(len(self.iteration_map)):
            if self.get_num_iters(i) < self.get_num_iters(i - 1):
                v[i] += self.dx
                self.iteration_map[i] += 1
                break
        return v

    def amplify_vector(self, v: List[int]):
        for i in range(len(v)):
            if v[i] != 0:
                v[i] += self.dx

    def get_num_iters(self, index: int) -> int:
        if index == -1:
            return 1
        return self.iteration_map[index]

    """
        :description: Simulates a game between engines p0, and p1.
        :return: a string, in list format, containing moves played and outcome.
        
        Outcome Values: -1 -> Black Wins. 0 -> Draw. 1 -> White Wins
    """

    def __play(self) -> (str, int):
        p1_turn = True
        game_log = init_game_log(self.move_seq)
        move = self.p0.send_move(self.move_seq)
        i = game_log.count('\n')
        game_log += str(i) + ". " + move + '\n'
        self.move_seq += move
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

    def play(self) -> str:
        if self.p0 is not None and self.p1 is not None:
            return self.__play()[0]
        else:
            print("Could not simulate game as either p0 or p1 is None")
            return ''


if __name__ == '__main__':
    c = Coach()
    c.clean_data()
    c.p0 = engine_factory()
    print("Elo: " + str(c.test()))
