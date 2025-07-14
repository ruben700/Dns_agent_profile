# agent_functions/sleep.py

from mythic_container.MythicCommandBase import (
    CommandBase,
    TaskArguments,
    CommandParameter,
    ParameterType,
    ParameterGroupInfo,
    CommandAttributes,
    SupportedOS,
    MythicTask,
    AgentResponse,
)

class SleepArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = [
            CommandParameter(
                name="seconds",
                cli_name="seconds",
                display_name="Seconds",
                type=ParameterType.Number,
                description="Número de segundos para dormir",
                parameter_group_info=[ParameterGroupInfo(ui_position=1, required=True)],
            )
        ]

    async def parse_arguments(self):
        if self.command_line and not self.command_line.startswith("{"):
            self.add_arg("seconds", int(self.command_line))
        else:
            self.load_args_from_json_string(self.command_line)

class SleepCommand(CommandBase):
    cmd              = "sleep"
    needs_admin      = False
    help_cmd         = "sleep <seconds>"
    description      = "Modifica el intervalo de sleep del agente"
    version          = 1
    author           = "Ruben"
    argument_class   = SleepArguments
    attackmapping    = []
    supported_ui_features = ["callback_table:sleep"]
    attributes       = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )

    async def create_tasking(self, task: SleepArguments) -> SleepArguments:
        task.display_params = str(task.args.get_arg("seconds"))
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
