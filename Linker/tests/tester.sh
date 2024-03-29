#!/bin/bash

function contains()
{
	local n=$#
	local value=${!n}
	for((i = 1; i < $#; i++)) {
		if [ "${!i}" == "${value}" ]; then
			echo "1"
			return 0
		fi
	}
	echo "0"
	return 0
}

function run_diff_test()
{
	# we pipe the directory listing through sort to ensure all tests are done in numerical order
	let passed_diffs=0
	for input in $(ls "${input_testdir}"/input-*|sort -nk2 -t'-')
	do
		for suffix in ${input##*-}
		do
			echo "---------- TEST $suffix ----------"
			if $show_all || (! $show_all && [ $(contains ${suppress_diffs[@]} "${suffix}") != 1 ]); then
				./"${binary}" ${opts} "${input}" >"${tmpfile}"
				diff -b -B -E "${tmpfile}" "${output_testdir}"/out-"${suffix}"
			fi
		done
	done
}

function run_leak_test()
{
	# we pipe the directory listing through sort to ensure all tests are done in numerical order
	for input in $(ls "${input_testdir}"/input-*|sort -nk2 -t'-')
	do
		for suffix in ${input##*-}
		do
			echo "---------- TEST $suffix ----------"
			if $show_all || (! $show_all && [ $(contains ${suppress_leaks[@]} $suffix) != 1 ]); then
				valgrind ./"${binary}" ${opts} "${input}" 2>"${tmpfile}" >/dev/null
				grep -A5 'LEAK SUMMARY' "${tmpfile}"|grep -v '0 bytes in 0 blocks'|cut -d' ' -f2-
				tail -1 "${tmpfile}"|grep -v '0 errors from 0 contexts'|cut -d' ' -f2-
			fi
		done
	done
}

function run_benchmark()
{
	for input in "${input_testdir}"/input*
	do
		for suffix in $(ls ${input}|cut -d- -f2)
		do
			./"${binary}" ${opts} "${input}" > /dev/null
		done
	done
}

function main()
{
	local binary=linker				# binary file name
	local opts=					# options to pass to binary program
	local input_testdir=../tests/inputs		# directory containing sample inputs
	local output_testdir=../tests/references	# directory containing reference outputs

	local diff_test=false				# perform output comparison tests
	local leak_test=false				# perform memory leak tests
	local benchmark=false				# perform benchmark
	local verbose=false				# increase verbosity
	local count=1					# default number of times to run benchmark test

	local suppress_diffs=( 9 )			# don't show diff errors for tests in array
	local suppress_leaks=(11 12 13 14 15 16 17 18)	# don't show memory leak errors for tests in array
	local show_all=true				# ignore suppress* arrays and print all errors
	local tmpfile=$(mktemp)

	while getopts "b:d:e:hmor:sv" opt; do
		case ${opt} in
			b)	# redefine binary
				binary="${OPTARG}"
				;;
			d)	# redefine input_testdir
				input_testdir="${OPTARG}"
				;;
			e)	# redefine output_testdir
				output_testdir="${OPTARG}"
				;;
			h)	# help message
				cat << EOF
Usage: $0 [OPTION]... output|memory|benchmark
Options: -bARG: set binary name to ARG
	-dARG: set directory containing test input files to ARG
	-eARG: set directory containing test output files to ARG
	-h: show this help dialog
	-rARG: number of runs over which to benchmark application
	-s: suppress test results from tests in hardcoded suppress* arrays
	-v: enable verbose mode for benchmarking
EOF
				exit 0
				;;
			r)	# number of benchmarking runs
				count="${OPTARG}"
				;;
			s)	show_all=false
				;;
			v)	# verbosity
				verbose=true
				;;
			?)
				echo "Unknown option, exiting now" >&2
				exit 1
		esac
	done
	while [ $# != 0 ]; do
		action=$1; shift
		if [ $action == 'output' ]; then
			diff_test=true
		fi
		if [ $action == 'memory' ]; then
			leak_test=true
		fi
		if [ $action == 'benchmark' ]; then
			benchmark=true
		fi
	done

	if [ ! -f "${binary}" ] || [ ! -x "${binary}" ]; then
		echo "Invalid program: '${binary}' does not exist or is not executable" >&2
		exit 1
	fi
	if [ ! -d "${input_testdir}" ] || [ ! -r "${input_testdir}" ]; then
		echo "Invalid input testdir: '${input_testdir}' does not exist or is not searchable" >&2
		exit 1
	fi
	if [ ! -d "${output_testdir}" ] || [ ! -r "${output_testdir}" ]; then
		echo "Invalid output testdir: '${output_testdir}' does not exist or is not searchable" >&2
		exit 1
	fi

	if ! $diff_test && ! $leak_test && ! $benchmark; then
		diff_test=true
		leak_test=true
		benchmark=true
	fi
	if $diff_test; then
		echo -e "\n\x1B[3mRunning diff tests...\x1B[0m\n"
		run_diff_test
	fi
	if $leak_test; then
		which valgrind >/dev/null
		if [ $? != 0 ]; then
			echo -e "\n\x1B[31;1mError: valgrind not found in path. Will not check for memory leaks\x1B[0m" >&2
		else
			echo -e "\n\x1B[3mRunning memory leak tests...\x1B[0m\n"
			run_leak_test
		fi
	fi

	if $benchmark; then
		echo -e "\n\x1B[3mRunning benchmark...\x1B[0m\n"
		let total=0
		for((i = 0; i < $count; i++)) {
			output=$((time run_benchmark) |& grep real)
			min=$(echo $output|cut -b5-|cut -dm -f1)
			sec=10#$(echo $output|cut -dm -f2|cut -d. -f1)
			milli=10#$(echo ${output##*m*.}|cut -ds -f1)
			let total=total+min*60000+sec*1000+milli
			if $verbose; then
				echo ${min}m$sec.${milli}s
			fi
		}
		let time_secs=total/1000
		let time_milli=total%1000
		let average=total/count
		let average_secs=average/1000
		let average_milli=average%1000
		echo -e "\n$count executions"
		echo -e "Total time:\t\t$time_secs.${time_milli}s"
		echo -e "Average runtime:\t$average_secs.${average_milli}s"
	fi

	if $diff_test || $leak_test; then
		echo "SUMMARY:"
		# !!! NON-GENERIC !!!
		let numtests=$(echo "${algos}"|tr -d ' \n'|wc -c)*5	# 5 test cases per algorithm
		if $diff_test; then
			echo "Passed diff tests: $passed_diffs/$numtests"
		fi
	#	if $leak_test; then
	#		echo "Passed diff tests: $passed_diffs/$numtests"
	#	fi
	fi

	rm -f $tmpfile
}

if [[ "$BASH_SOURCE" == "$0" ]]
then
	main $*
fi

