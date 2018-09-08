#CD
Host-OS directory change/query, CP/M drive to host-OS directory mapping tool

    CD           List drive assignments
    CD A:        Show drive assignemnt only a specific drive, A: in this case
    CD A: path   Assign/change a host-OS directory to a CP/M drive (A: here)
    CD path      Change a host-OS directory assigned to the current CP/M drive
    CD B: -      Delete assignment for a given CP/M drive

where 'path' is the path on the host-OS, on Windows, it can even carry drive
letter, but it's a different one (CP/M versus Windows drives)

This command allows you to deal with emulation-specific aspects of the CP/M
file system. In reCPM, the original CP/M filesystem is not emulated at block
I/O level, rather then the host-OS (the OS runs thish emulator) file system
is used at FCB level only. CP/M 2.x does not know about directioes at all.
Though, it know the notion of drives like A:. In fact, this is the source
of drive names even in Windows till the very current day. To bridge the
the difference, we can assign a given host-OS directory to a given CP/M drive.
