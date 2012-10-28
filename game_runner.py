#!/usr/bin/env python
# vim: set expandtab ts=2 sw=2 tw=80 fo=cqt wm=0

AGENT_INVOCATION_COMMANDS = (
  './quarto_agent -random',
  './quarto_agent -novice',
  './quarto_agent -minmax 1',
  './quarto_agent -minmax 2',
  './quarto_agent -minmax 3',
  './quarto_agent -minmax 4',
  './quarto_agent -minmax 6',
  './quarto_agent -minmax 100',
)








import itertools
import os
import random
import subprocess
import sys


ROW_COLS = tuple([(row, col) for row in range(4) for col in range(4)])
POSSIBLE_QUARTO_POSITIONS = (
  ((0, 0), (0, 1), (0, 2), (0, 3)),
  ((1, 0), (1, 1), (1, 2), (1, 3)),
  ((2, 0), (2, 1), (2, 2), (2, 3)),
  ((3, 0), (3, 1), (3, 2), (3, 3)),

  ((0, 0), (1, 0), (2, 0), (3, 0)),
  ((0, 1), (1, 1), (2, 1), (3, 1)),
  ((0, 2), (1, 2), (2, 2), (3, 2)),
  ((0, 3), (1, 3), (2, 3), (3, 3)),
  
  ((0, 0), (1, 1), (2, 2), (3, 3)),
  ((0, 3), (1, 2), (2, 1), (3, 0)),
)


class Agent(object):
  """
  Facade for agent program.
  Communicates with agent's stdin and stdout over pipes.
  """
  def __init__(self, command):
    self._command = command
    self._process = subprocess.Popen(command, shell=True, 
                                     stdin=subprocess.PIPE, 
                                     stdout=subprocess.PIPE)
  
  def act(self, board, cur_piece):
    for row, col in ROW_COLS:
      self._process.stdin.write('%d ' % (board[(row, col,)]
                                        if (row, col,) in board else -1))
    self._process.stdin.write('\n')
    self._process.stdin.write('%d\n' % cur_piece)
    self._process.stdin.flush()
    choice = map(int, self._process.stdout.readline().split())
    return (choice[0], choice[1],), choice[2]

  __str__ = lambda self: 'Agent[%s]' % self._command




##################################################
#### PER-MATCH LOGIC AND REPORTING PROCEDURES ####
##################################################

def is_win(board):
  for qp in POSSIBLE_QUARTO_POSITIONS:
    all_taken = all(x in board for x in qp)
    if all_taken:
      positives = board[qp[0]] & board[qp[1]] & board[qp[2]] & board[qp[3]]
      negatives = board[qp[0]] | board[qp[1]] | board[qp[2]] | board[qp[3]]
      if positives != 0 or negatives != 15:
        return True                     
  return False                        

def piece_to_str(piece):
  return ''.join('X' if (piece&x) else 'x' for x in (1,2,3,4,))

def print_board(board):               
  print '\n  0    1    2    3'
  for row in range(4):
    print row,
    for col in range(4):
      if (row, col) in board:
        print '%-4s' % piece_to_str(board[(row,col)]),
      else:
        print '-   ',
    print 
  print 

def report_game_state(step_num, cur_player, cur_piece,
                      move_row, move_col, next_piece):
  print '\nSTEP %d    %s acts...' % (step_num, cur_player,)
  print_board(board)
  print '  %s places %s at (%d,%d) and selects %s as next piece to play...' % (
                cur_player, piece_to_str(cur_piece), 
                move_row, move_col, piece_to_str(next_piece),)

def run_quarto_match(agent_a, agent_b, first_player):      
  board, cur_piece = {}, -1
  cur_player = first_player

  for step_num in itertools.count(1):
    (move_row, move_col), next_piece =(cur_player.act(board, cur_piece))

    report_game_state(step_num, cur_player, cur_piece,
                      move_row, move_col, next_piece)

    is_valid_move = (
          ((len(board) == 0 and move_row == -1 and move_col == -1) 
           or 
           (0 <= move_row < 16 and 0 <= move_col < 16))
      and (0 <= next_piece < 16 or 
           (len(board) == 15 and next_piece == -1))
      and ((move_row, move_col,) not in board.keys())
      and (not next_piece in board.values())
    )
    if not is_valid_move:
      print cur_player, 'made an illegal move! Aborting game.'
      sys.exit(0)

    if move_row != -1:
      board[(move_row, move_col,)] = cur_piece
    cur_piece = next_piece
    print_board(board)
    
    if is_win(board):
      return 1 if cur_player == agent_a else -1
    elif len(board) == 16:
      # Game tied.
      return 0 
    
    cur_player = agent_a if cur_player == agent_b else agent_b




###################################
####  TOP-LEVEL GAME RUNNING   ####
###################################

def read_data(prompt, trans=lambda x: x):
  return trans(raw_input(prompt))

def print_agent_list():
  print '---+++---+++---+++--- AGENT_INVOCATION_COMMANDS ---+++---+++---+++---' 
  print '\n'.join(' %d: %s' % (i, a,) 
                  for i, a in enumerate(AGENT_INVOCATION_COMMANDS))
  print '---+++---+++---+++---~~~~~~~~---+++---+++---+++---' 

def report_win(player):     
  print 3*('---+++---+++---+++--- %s WINS ---+++---+++---+++---\n' % player),

def report_tie():     
  print 3*'---+++---+++---+++--- TIED~TIED~TIED ---+++---+++---+++---' 

def run_quarto_game():
  agent_a, agent_b = None, None
  num_matches = 0

  def setup_from_stdin():
    print_agent_list()
    def read_agent_choice(s):
      return read_data('Choose ' + s + ' agent: ', 
                       lambda i: Agent(AGENT_INVOCATION_COMMANDS[int(i)]))
    agent_a = read_agent_choice('first')
    agent_b = read_agent_choice('second')
    num_matches = read_data('Choose number of matches: ', int)

  def setup_from_argv():
    try:
      agent_a = Agent(AGENT_INVOCATION_COMMANDS[int(sys.argv[1])])
      agent_b = Agent(AGENT_INVOCATION_COMMANDS[int(sys.argv[2])])
      num_matches = int(sys.argv[3])
    except:
      print 'Usage %s ([agent a id] [agent b id] [num matches])' % sys.argv[0]
      sys.exit(-1)
  
  def spin():
    print '\nPlaying %d matches between %s and %s...' % (num_matches, 
                                                         agent_a, agent_b,)
    num_wins_a = num_wins_b = num_ties = 0
    for match_num in range(1, num_matches+1):
      print 'Starting match %d...' % match_num
      match_result = run_quarto_match(agent_a, agent_b, 
                                      agent_a if (match_num&1) else agent_b)
      if match_result ==  1: 
        num_wins_a += 1
        report_win(agent_a)
      elif match_result == -1: 
        num_wins_b += 1
        report_win(agent_b)
      else:
        num_ties += 1
      print '\n'*10  

    print '%-15s won %d matches' % (agent_a, num_wins_a,)
    print '%-15s won %d matches' % (agent_b, num_wins_b,)
    print 'game tied in %d matches' % num_ties

  if len(sys.argv) == 1:
    setup_from_stdin()
  else:
    setup_from_argv()
  spin()




if __name__ == '__main__':
  run_quarto_game()
