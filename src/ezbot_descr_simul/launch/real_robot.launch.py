# copyright option3 ensma
import os

from ament_index_python.packages import get_package_share_directory


from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription, TimerAction
from launch.actions import RegisterEventHandler
from launch.event_handlers import OnProcessStart
from launch.actions import DeclareLaunchArgument
import launch_ros.actions
from launch.substitutions import LaunchConfiguration
from launch.launch_description_sources import PythonLaunchDescriptionSource

from launch_ros.actions import Node
from launch.substitutions import Command



def generate_launch_description():


    logger = LaunchConfiguration('log_level')  

    # Include the robot_state_publisher launch file, provided by our own package. Force sim time to be enabled
    # !!! MAKE SURE YOU SET THE PACKAGE NAME CORRECTLY !!!

    package_name='ezbot_descr_simul' #<--- CHANGE ME

    rsp = IncludeLaunchDescription(
                PythonLaunchDescriptionSource([os.path.join(
                    get_package_share_directory(package_name),'launch','rsp.launch.py'
                )]), launch_arguments={'use_sim_time': 'false'}.items()
    )

    joystick = IncludeLaunchDescription(
        PythonLaunchDescriptionSource([os.path.join(
            get_package_share_directory("teleop_twist_joy"),'launch','teleop-launch.py'
        )]), launch_arguments={'config_filepath': '/home/vincent/joystick.yaml','joy_vel':'/omnidirectional_controller/cmd_vel_unstamped'}.items(),
        
    )

    homemade_controller = Node(
        package='ezbot_descr_simul',
        executable='homemade_controller',
        name='homemade_controller',
        output='screen',
        parameters=[{'log_level': logger}]
    )

    return LaunchDescription([
        rsp,
        joystick,
        homemade_controller
        
    ])