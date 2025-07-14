# agent_functions/cp.py

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

class CpArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = [
            CommandParameter(
                name="source",
                cli_name="source",
                display_name="Source",
                type=ParameterType.String,
                description="Archivo origen",
                parameter_group_info=[ParameterGroupInfo(ui_position=1, required=True)],
            ),
            CommandParameter(
                name="destination",
                cli_name="destination",
                display_name="Destination",
                type=ParameterType.String,
                description="Archivo destino",
                parameter_group_info=[ParameterGroupInfo(ui_position=2, required=True)],
            )
        ]

    async def parse_arguments(self):
        parts = self.command_line.split()
        if len(parts) == 2:
            self.add_arg("source", parts[0])
            self.add_arg("destination", parts[1])
        else:
            self.load_args_from_json_string(self.command_line)

class CpCommand(CommandBase):
    cmd              = "cp"
    needs_admin      = False
    help_cmd         = "cp <source> <destination>"
    description      = "Copia un archivo"
    version          = 1
    author           = "Ruben"
    argument_class   = CpArguments
    attackmapping    = ["T1106"]
    supported_ui_features = ["callback_table:cp"]  
    attributes       = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )

    async def create_tasking(self, task: CpArguments) -> CpArguments:
        task.display_params = f"{task.args.get_arg('source')} → {task.args.get_arg('destination')}"
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
