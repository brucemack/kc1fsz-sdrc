# Environment Setup

Setup virtual environment:

    python3 -m venv dev

Activate virtual environment:

    . dev/bin/activate

Install packages:

    pip install -r requirements.txt

PYTHONPATH is needed to reference the firpm-py module in adjacent repo:
        
    export PYTHONPATH=../../../firpm-py:.
