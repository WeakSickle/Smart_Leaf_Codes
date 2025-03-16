# this is a class that can be used to interface simply with the config file

import json
from pathlib import Path


class config_interface:
    def __init__(self):
        script_dir = Path(__file__).resolve().parent
        parent_dir = script_dir.parent 
        self.file_path = parent_dir / 'config.json'
        self.load_config(self.file_path)
        pass

    def load_config(self, filename):
        """
        Load the config file and return its contents as a dictionary.
        """
        try:
            with open(filename, 'r') as file:
                return json.load(file)
        except FileNotFoundError:
            raise FileNotFoundError(f"Config file not found at: {filename}")
        except json.JSONDecodeError:
            raise ValueError(f"Invalid JSON in config file: {filename}")

    def get_config_value(self, value):
        '''returns a given setting from the config file'''
        # Get a value from the config file
        return self.config[value]
    
