import os
import time

from subprocess import Popen, PIPE

project_dir = "C:\\Users\\Alan Tao\\Desktop\\Education\\Projects\\C\\juliette"
source_dir = project_dir + '\\src'
tune_dir = project_dir + '\\tune'


class Engine:

    def __init__(self, binary: str):
        self.bin = binary
        self.connection = None
        self.move_sequence = ""

    def __del__(self):
        os.chdir(tune_dir + '\\data')
        os.system('del ' + self.bin + '.exe')

    """
    :param move: String representation of opponent engine's move
    :return str: String representation of engine's move
    
    Description: Sends a move of the engine's opponent to the engine, returns engine's reply.
    """

    def send_move(self, move_seq: str) -> str:
        os.chdir(tune_dir + '\\data')
        self.connection = Popen([self.bin, 'tune', move_seq], stdout=PIPE, stdin=PIPE)
        own_move = str(self.connection.communicate(input=move_seq.encode())[0])[2:-1:]
        return own_move


def engine_factory() -> Engine:
    os.chdir(source_dir)
    output_bin = 'juliette' + str(time.time())[-4:]
    os.system('g++ *.cpp -lWS2_32 -o ' + output_bin)
    os.system('move ' + output_bin + '.exe ..\\tune\\data')
    engine = Engine(output_bin)
    return engine
