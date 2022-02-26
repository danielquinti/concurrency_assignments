-module(comp).

-export([comp/1, comp/2, comp_proc/2, comp_proc/3,compression_function/2,  decomp/1, decomp/2, decomp_proc/2, decomp_proc/3, decompression_function/2]).


-define(DEFAULT_CHUNK_SIZE, 1024*1024).

%%% File Compression

comp(File) -> 
%% Compress file to file.ch
    comp(File, ?DEFAULT_CHUNK_SIZE).

comp(File, Chunk_Size) ->  
%% Starts a reader and a writer which run in separate processes
    case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader} ->
            case archive:start_archive_writer(File++".ch") of
                {ok, Writer} ->
                    comp_loop(Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.


comp_loop(Reader, Writer) ->  
%% Compression loop => get a chunk, compress it, send to writer
    Reader ! {get_chunk, self()},  
    %% request a chunk from the file reader
    receive
        {chunk, Num, Offset, Data} ->   
	%% got one, compress and send to writer
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            comp_loop(Reader, Writer);
        eof ->  
	%% end of file, stop reader and writer
            Reader ! stop,
            Writer ! stop;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n",[Reason]),
            Reader ! stop,
            Writer ! abort
    end.

comp_proc(File,Procs) ->
%% Compress file to file.ch
    spawn(fun () -> comp_aux(File,?DEFAULT_CHUNK_SIZE,Procs) end).

comp_proc(File,Chunk_Size,Procs) ->
%% Compress file to file.ch
    spawn(fun () -> comp_aux(File,Chunk_Size,Procs) end).

comp_aux(File, Chunk_Size,Procs) ->  
%% Starts a reader and a writer which run in separate processes
    case file_service:start_file_reader(File, Chunk_Size) of
        {ok, Reader} ->
            case archive:start_archive_writer(File++".ch") of
                {ok, Writer} ->
			    register(god, self()),
			    process_flag(trap_exit, true),
			    compression_process_creator(Reader, Writer, Procs),
			    wait(Procs,Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

compression_process_creator(Reader, Writer, Procs) ->
	if
		Procs > 0 -> spawn_link(comp, compression_function,[Reader, Writer]),
		compression_process_creator(Reader, Writer, Procs - 1);

		Procs == 0 -> ok
	end.

wait(Procs, Reader, Writer) ->
%% Wait for all processes to finish
	if
		Procs > 0 -> receive
				     finish -> wait(Procs - 1, Reader, Writer)
			     end;
		true -> Reader ! stop,
            		Writer ! stop
	end.

compression_function(Reader, Writer) ->  
%% Compression loop => get a chunk, compress it, send to writer
    Reader ! {get_chunk, self()},  
    %% request a chunk from the file reader
    receive
        {chunk, Num, Offset, Data} ->   
	%% got one, compress and send to writer
            Comp_Data = compress:compress(Data),
            Writer ! {add_chunk, Num, Offset, Comp_Data},
            compression_function(Reader, Writer);
        eof ->  
	%% end of file, stop reader and writer
	    god ! finish;
	    
        {error, Reason} ->
            io:format("Error reading input file: ~w~n",[Reason]),
            Reader ! stop,
            Writer ! abort
    end.


%% File Decompression

decomp_proc(Archive, Procs) ->
    decomp_proc(Archive, string:replace(Archive, ".ch", "", trailing),Procs).

decomp_proc(Archive, Output_File,Procs) ->
    case archive:start_archive_reader(Archive) of
        {ok, Reader} ->
            case file_service:start_file_writer(Output_File) of
                {ok, Writer} ->
                    register(master, self()),
		    process_flag(trap_exit, true),
		    decompression_process_creator(Reader, Writer, Procs),
		    wait(Procs, Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

decompression_process_creator(Reader, Writer, Procs) ->
	if
		Procs > 0 -> spawn_link(comp, decompression_function, [Reader, Writer]),
		decompression_process_creator(Reader, Writer, Procs - 1);

		Procs == 0 -> ok
	end.

decompression_function(Reader, Writer) ->
    Reader ! {get_chunk, self()},
    %% request a chunk from the reader
    receive
        {chunk, _Num, Offset, Comp_Data} ->  %% got one
            Data = compress:decompress(Comp_Data),
            Writer ! {write_chunk, Offset, Data},
            decompression_function(Reader, Writer);
        eof ->  
		master ! finish;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n", [Reason]),
            Writer ! abort,
            Reader ! stop
    end.

decomp(Archive) ->
    decomp(Archive, string:replace(Archive, ".ch", "", trailing)).

decomp(Archive, Output_File) ->
    case archive:start_archive_reader(Archive) of
        {ok, Reader} ->
            case file_service:start_file_writer(Output_File) of
                {ok, Writer} ->
                    decomp_loop(Reader, Writer);
                {error, Reason} ->
                    io:format("Could not open output file: ~w~n", [Reason])
            end;
        {error, Reason} ->
            io:format("Could not open input file: ~w~n", [Reason])
    end.

decomp_loop(Reader, Writer) ->
    Reader ! {get_chunk, self()},
    %% request a chunk from the reader
    receive
        {chunk, _Num, Offset, Comp_Data} ->  %% got one
            Data = compress:decompress(Comp_Data),
            Writer ! {write_chunk, Offset, Data},
            decomp_loop(Reader, Writer);
        eof ->  
	%% end of file => exit decompression
            Reader ! stop,
            Writer ! stop;
        {error, Reason} ->
            io:format("Error reading input file: ~w~n", [Reason]),
            Writer ! abort,
            Reader ! stop
    end.
