#!/usr/bin/env bash

echo "Running metric calculation tests..."
python3 -m unittest --verbose tests.test_metric_calculation

echo "Running bounds calculation tests..."
python3 -m unittest --verbose tests.test_metric_bounds
