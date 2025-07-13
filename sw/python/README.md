# Environment Setup

For WSL:

* Make sure you are running WSL2
* Make sure that Python TK is installed:

    sudo apt-get install python3-tk

Setup virtual environment:

    python3 -m venv dev

Activate virtual environment:

    . dev/bin/activate

Install packages:

    pip install -r requirements.txt

PYTHONPATH is needed to reference the firpm-py module in adjacent repo:
        
    export PYTHONPATH=../../../firpm-py:.
