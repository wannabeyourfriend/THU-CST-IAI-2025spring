import os

class time:
	def __init__(self, mine, his):
		self.mine = mine
		self.his = his

class result:
	def __init__(self):
		self.res = {	'wina'		:	0,	#self's win as offensive
						'winb'		:	0,	#self's win as deffensive
						'losea'		:	0,	#self's lose as offensive
						'loseb'		:	0,	#selfs' lose as deffensive
						'tie'		:	0,	#tie
						'bug'		:	0,	#self's bug
						'debug'		:	0,	#opponent's bug
						'illegal'	:	0,	#self's illegal point
						'deillegal'	:	0,	#opponent's illegal point
						'timeout'	:	0,	#self's timeout
						'detimeout'	:	0,	#opponent's timeout
						'loaderr'	:	0,	#self's load file err
						'deloaderr'	:	0,	#opponent's load file err
						'noentry'	:	0,	#self's find function entry err
						'denoentry'	:	0	#opponent's find function entry err
					}
		self.tieTimeList = []
	
idList = {}


def process(line, A, lineNo):
	# Ensure line is a string before splitting, though readline() should provide it.
	# It's possible an empty line from the default in process_block makes it here.
	if not isinstance(line, str):
		# Handle unexpected type if necessary, or rely on split to fail
		# For now, let's assume it's mostly correct due to default values
		pass

	parts = line.split('\t')
	# Add a check for expected number of parts if `line` could be malformed
	if len(parts) < 3:
		print(f"Warning: Malformed line for A='{A}', lineNo={lineNo}: '{line}'")
		# Decide how to handle this: skip, use defaults, or log & continue
		# For example, if it's critical, you might add to a 'malformed' counter
		# idList[A].res['malformed'] = idList[A].res.get('malformed', 0) + 1
		return # Or assign default error code

	# The original slicing assumed "ms" at the end.
	# If parts[2] could be shorter than 2 chars, this would error.
	# Adding a check for length.
	if len(parts[2]) >= 2:
		parts[2] = parts[2][:-2]
	else:
		# Handle case where parts[2] is too short, e.g., assign a default or log
		# print(f"Warning: parts[2] is too short for A='{A}', lineNo={lineNo}: '{parts[2]}'")
		pass # parts[2] will be used as is, or you can set it to '0' or some error indicator
	
	if not A in idList:
		idList[A] = result()
	
	if parts[0] == '0':
		idList[A].res['tie'] += 1
		# Ensure parts[1] and parts[2] are valid numbers before creating time object
		idList[A].tieTimeList.append(time(parts[1], parts[2])) # Assuming parts[1] and parts[2] are valid time strings
	
	elif parts[0] == '1':
		if lineNo == 1:
			idList[A].res['wina'] += 1
		elif lineNo == 2:
			idList[A].res['winb'] += 1
			
	elif parts[0] == '2':
		if lineNo == 1:
			idList[A].res['losea'] += 1
		elif lineNo == 2:
			idList[A].res['loseb'] += 1
	
	elif parts[0] == '3':
		idList[A].res['bug'] += 1
	
	elif parts[0] == '4':
		idList[A].res['illegal'] += 1
	
	elif parts[0] == '5':
		idList[A].res['debug'] += 1
	
	elif parts[0] == '6':
		idList[A].res['deillegal'] += 1
	
	elif parts[0] == '7':
		idList[A].res['timeout'] += 1
	
	elif parts[0] == '8':
		idList[A].res['detimeout'] += 1
	
	elif parts[0] == '-1':
		idList[A].res['loaderr'] += 1
	
	elif parts[0] == '-2':
		idList[A].res['deloaderr'] += 1
	
	elif parts[0] == '-3':
		idList[A].res['noentry'] += 1
	
	elif parts[0] == '-4':
		idList[A].res['denoentry'] += 1
	
	# Consider adding an else clause here to catch unexpected values in parts[0]
	# else:
	#   print(f"Warning: Unknown result code '{parts[0]}' for A='{A}', lineNo={lineNo}")
	#   idList[A].res['unknown_code'] = idList[A].res.get('unknown_code', 0) + 1

	return

def process_block(file, A):
	line = file.readline() # Read the "ID:" line
	if not line: # End of file
		return False
	line = line.strip()
	
	# Corrected line: removed .decode('utf-8')
	# Also, ensure line is not empty before trying to slice and check line[:-1]
	if not line or not line.endswith(':') or not (line[:-1].isnumeric() if len(line) > 1 else False) :
		# If line is just ":", line[:-1] is empty, isnumeric() is False, which is fine.
		# If line is empty or doesn't end with ':', it's an invalid block start.
		# print(f"Debug: Invalid block start or EOF reached. Line: '{line}'") # Optional debug
		return False

	# Read the first data line for the block
	data_line1 = file.readline()
	if not data_line1: # Unexpected EOF after a valid block header
		# print(f"Debug: Unexpected EOF for A='{A}' after block header. Treating as error.")
		process('-3\t0\t0', A, 1) # Process with default error
		process('-3\t0\t0', A, 2) # Process with default error for the second expected line
		return True # Or False if this should halt processing
	process(data_line1.strip(), A, 1)

	# Read the second data line for the block
	data_line2 = file.readline()
	if not data_line2: # Unexpected EOF
		# print(f"Debug: Unexpected EOF for A='{A}' after first data line. Treating as error.")
		process('-3\t0\t0', A, 2) # Process with default error
		return True # Or False
	process(data_line2.strip(), A, 2)
	
	# Read the expected empty line separator between blocks, if it exists
	# This line is not used other than advancing the file pointer.
	_ = file.readline() 
	return True

def main():	
	resdir = "./compete_result/"
	try:
		namelist = os.listdir(resdir)
	except FileNotFoundError:
		print(f"Error: Directory '{resdir}' not found.")
		return
	except Exception as e:
		print(f"Error listing directory '{resdir}': {e}")
		return

	for name in namelist:
		# Assuming filenames are long enough for this slice.
		# Add a check if necessary: if len(name) >= 10:
		A = name[0:10]
		filepath = os.path.join(resdir, name)
		try:
			with open(filepath, 'r') as f: # Use 'with' for automatic file closing
				flag = True
				while flag:
					flag = process_block(f, A)
		except FileNotFoundError:
			print(f"Error: File '{filepath}' not found during processing.")
			continue # Skip to the next file
		except Exception as e:
			print(f"Error processing file '{filepath}': {e}")
			continue # Skip to the next file


	output_filepath = "./stat.txt"
	try:
		with open(output_filepath, 'w') as resfile: # Use 'with' for automatic file closing
			resfile.write("id\twina\twinb\tlosea\tloseb\ttie\tbug\tdebug\tillegal\tdeillegal\ttimeout\tdetimeout\tloaderr\tdeloaderr\tnoentry\tdenoentry\n")
			for i in idList:
				res = idList[i].res
				# Ensure all keys exist, or use res.get(key, 0) for safety
				line_parts = [
					str(i),
					str(res.get('wina', 0)), str(res.get('winb', 0)),
					str(res.get('losea', 0)), str(res.get('loseb', 0)),
					str(res.get('tie', 0)), str(res.get('bug', 0)),
					str(res.get('debug', 0)), str(res.get('illegal', 0)),
					str(res.get('deillegal', 0)), str(res.get('timeout', 0)),
					str(res.get('detimeout', 0)), str(res.get('loaderr', 0)),
					str(res.get('deloaderr', 0)), str(res.get('noentry', 0)),
					str(res.get('denoentry', 0))
				]
				line = "\t".join(line_parts)
				resfile.write(line + "\n")
		print(f"Statistics successfully written to {output_filepath}")
	except IOError:
		print(f"Error: Could not write to file '{output_filepath}'. Check permissions or disk space.")
	except Exception as e:
		print(f"An unexpected error occurred while writing the output file: {e}")


if __name__ == '__main__':
	main()