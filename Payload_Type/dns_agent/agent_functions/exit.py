# agent_functions/exit.py

from mythic_container.MythicCommandBase import (
    CommandBase,
    TaskArguments,
    CommandAttributes,
    SupportedOS,
    MythicTask,
    AgentResponse,
)

class ExitArguments(TaskArguments):
    def __init__(self, command_line, **kwargs):
        super().__init__(command_line, **kwargs)
        self.args = []  # ningún parámetro

    async def parse_arguments(self):
        return

class ExitCommand(CommandBase):
    cmd = "exit"
    needs_admin = False
    help_cmd = "exit"
    description = "Detiene el bucle de comandos y sale"
    version = 1
    author = "Ruben"
    argument_class = ExitArguments
    attackmapping = []
    supported_ui_features = ["callback_table:exit"]  
    attributes = CommandAttributes(
        supported_os=[SupportedOS.Windows],
        builtin=True,
        suggested_command=True,
    )


    async def create_tasking(self, task: ExitArguments) -> ExitArguments:
        task.display_params = ""
        return task

    async def process_response(self, response: AgentResponse) -> AgentResponse:
        return response
